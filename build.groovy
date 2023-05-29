#!/usr/bin/env groovy

/* 
 * File:   build.groovy
 * Author: David Thacher
 * License: GPL 3.0
 */

@Grab(group='org.apache.commons', module='commons-lang3', version='3.11' )
import org.apache.commons.lang3.SystemUtils

/*
DEFINE_MULTIPLEX            32          "Multiplex Scan of panel"
DEFINE_MAX_RGB_LED_STEPS    8000        "Contrast Ratio - Min LED const forward current in nA divided by min light current in nA"
DEFINE_MAX_REFRESH          3840        "Max refresh of panel"
DEFINE_COLUMNS              128         "Chain length"
DEFINE_SERIAL_CLOCK         15.625      "HUB75 serial clock speed in MHz"
DEFINE_BLANK_TIME           1           "Multiplex blanking time in uS"
DEFINE_RED_GAIN             1.0         "Gain for Red LED (TLC5958 only)"
DEFINE_GREEN_GAIN           1.0         "Gain for Green LED (TLC5958 only)"
DEFINE_BLUE_GAIN            1.0         "Gain for Blue LED (TLC5958 only)"
*/

apps = ""
flags = ""

def handle_serial_algorithm(cfg, index) {
    name = cfg.build[index].attributes().name
    c = cfg.build[index].serial[0].attributes()
    
    if (c.algorithm == "uart") {
        s = cfg.build[index].serial[0].uart[0].attributes()
        flags += sprintf(" -D%s_DEFINE_SERIAL_UART_BAUD=%s -D%s_DEFINE_SERIAL_RGB_type=%s", name, s.baud, name, s.RGB_type)
    }
    else {
        println "Unknown serial algorithm " + c.algorithm
        System.exit(1)
    }
}

def handle_matrix_algorithm(cfg, index) {
    name = cfg.build[index].attributes().name
    c = cfg.build[index].matrix[0].attributes()

    if (c.algorithm == "BCM" || c.algorithm == "PWM" || c.algorithm == "ON_OFF")
        flags += sprintf(" -D%s_DEFINE_MULTIPLEX=%s -D%s_DEFINE_MULTIPLEX_NAME=%s -D%s_DEFINE_MAX_RGB_LED_STEPS=%s -D%s_DEFINE_MAX_REFRESH=%s -D%s_DEFINE_COLUMNS=%s -D%s_DEFINE_SERIAL_CLOCK=%s -D%s_DEFINE_BLANK_TIME=%s -D%s_DEFINE_ALGORITHM=%s", name, c.serial_algorithm, name, c.multiplex, name, c.multiplex_name, name, c.max_rgb_led_steps, name, c.max_refresh, name, c.columns, name, c.serial_clock, name, c.blank_time, name, c.matrix_algorithm, name)
    else if (c.algorithm == "TLC5958") {
        m = cfg.build[index].matrix[0].TLC5958[0].attributes()
        flags += sprintf(" -D%s_DEFINE_MULTIPLEX=%s -D%s_DEFINE_MULTIPLEX_NAME=%s -D%s_DEFINE_MAX_RGB_LED_STEPS=%s -D%s_DEFINE_MAX_REFRESH=%s -D%s_DEFINE_COLUMNS=%s -D%s_DEFINE_SERIAL_CLOCK=%s -D%s_DEFINE_BLANK_TIME=%s -D%s_DEFINE_ALGORITHM=%s", name, c.serial_algorithm, name, c.multiplex, name, c.multiplex_name, name, c.max_rgb_led_steps, name, c.max_refresh, name, c.columns, name, c.serial_clock, name, c.blank_time, name, c.matrix_algorithm, name)
        flags += sprintf(" -D%s_DEFINE_FPS=%s -D%s_DEFINE_GCLK=%s -D%s_DEFINE_RED_GAIN=%s -D%s_DEFINE_GREEN_GAIN=%s -D%s_DEFINE_BLUE_GAIN=%s", name, m.fps, name, m.gclk, name, m.red_gain, name, m.green_gain, name, m.blue_gain)
    }
    else if (c.algorithm == "TLC5946") {
        m = cfg.build[index].matrix[0].TLC5946[0].attributes()
        flags += sprintf(" -D%s_DEFINE_MULTIPLEX=%s -D%s_DEFINE_MULTIPLEX_NAME=%s -D%s_DEFINE_MAX_RGB_LED_STEPS=%s -D%s_DEFINE_MAX_REFRESH=%s -D%s_DEFINE_COLUMNS=%s -D%s_DEFINE_SERIAL_CLOCK=%s -D%s_DEFINE_BLANK_TIME=%s -D%s_DEFINE_ALGORITHM=%s", name, c.serial_algorithm, name, c.multiplex, name, c.multiplex_name, name, c.max_rgb_led_steps, name, c.max_refresh, name, c.columns, name, c.serial_clock, name, c.blank_time, name, c.matrix_algorithm, name)
        flags += sprintf(" -D%s_DEFINE_GCLK=%s", name, m.gclk)
    }
    else {
        println "Unknown matrix algorithm " + c.algorithm
        System.exit(1)
    }
}

def build_flavor(cfg, index) {
    c = cfg.build[index].attributes()

    if (c.enable == "true") {   
        if (apps == "")
            apps = c.name
        else
            apps += sprintf(";%s", c.name)

        handle_serial_algorithm(cfg, index)
        handle_matrix_algorithm(cfg, index) 
    }
}

def build_linux(run) {
    String script = "./LED_Matrix/build/build.sh"
    new File("./LED_Matrix/build/").mkdirs()
    File f = new File(script)
    f.text = ""
    f << "#!/bin/bash\n"
    f << "cd ./LED_Matrix\n"
    f << "DIR=\$(pwd)\n"
    f << "export PICO_SDK_PATH=\$DIR/lib/pico-sdk\n"
    f << "cp \$DIR/lib/pico-sdk/external/pico_sdk_import.cmake .\n"
    f << "cd build\n"
    f << sprintf("cmake .. -DAPPS=\"%s\" %s  2>&1\n", apps, flags)
    f << "make -j \$((\$(nproc) * 2)) 2>&1 #VERBOSE=1\n"
    f << "if [ \$? -eq 0 ]; then\n"
    f << "\techo \"Binary output:\"\n"
    f << "\tls -la \$DIR/build/src/led_*.*\n"
    f << "else\n"
    f << "\texit 1\n"
    f << "fi\n"
    ("chmod +x " + script).execute().waitFor()

    if (run) {
        def proc = script.execute()
        println proc.text
        System.exit(proc.exitValue())
    }
    else {
        print "Please run:\n"
        println script
    }
}

def build(run) {
    if (SystemUtils.IS_OS_LINUX)
        return build_linux(run)
    else
        return "Unsupported Build Platform\n"
}

def cli = new CliBuilder(usage: 'build.groovy [-h] -c <cfg.xml>')
cli.with { 
    h longOpt: 'help', 'Show usage information' 
    c longOpt: 'config', args:1, argName: 'cfg.xml', required: true, 'XML build configuration file'
    r longOpt: 'run', 'Execute the script'
}
def options = cli.parse(args)
if (!options) return
if (options.h) cli.usage()

def cfg = new XmlSlurper().parse(new File(options.c))
cfg.build.eachWithIndex { it, index -> build_flavor(cfg, index) }
build(options.r)

