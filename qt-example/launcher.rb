#! /usr/bin/env ruby

require 'open3'
require 'io/console'
require 'json'

class Runner
    attr_accessor :binpath

    def initialize(binary)
        @binpath = binary

        @status = nil
        @thread = nil
    end

    def run
        stdin, @stdout, @stderr, @thread = Open3.popen3(@binpath)
        stdin.close()
        true
    rescue
        STDERR.puts "Failed to launch #{@binpath}"
        false
    end

    def running?
        rs, ws, = IO.select([@stdout, @stderr], nil, nil, 0)
        if rs && r = rs[0]
            STDERR.print r.read(r.stat.size)
        end
        @thread && @thread.alive?
    end

    def kill(signame)
        Process.kill(signame, @thread.pid) if running?()
    end

    def wait(timeout=nil)
        @thread.join(timeout) != nil
    end

    def status
        @thread.value
    end
end

def rebuild(runner)
    bindir = File.dirname(runner.binpath)
    binfile = File.basename(runner.binpath)
    if File.exist?(cmakepath = File.join(bindir, "CMakeFiles"))
        autogendir = File.join(cmakepath, "#{binfile}_autogen.dir")
        autogeninfo = File.join(autogendir, "AutogenInfo.json")
        if File.exist?(autogeninfo)
            autogeninfo = JSON.parse(File.read(autogeninfo))
            `#{autogeninfo["CMAKE_EXECUTABLE"]} --build #{autogeninfo["CMAKE_BINARY_DIR"]}`
        else
            STDERR.puts "AutogenInfo.json not found, can't rebuild #{runner.binpath}"
        end
    end
end

def main
    if ARGV.length < 1
        STDERR.puts "Usage: #{File.basename($PROGRAM_NAME)} <binpath>"
        exit 1
    end

    runner = Runner.new(ARGV[0])
    if !runner.run()
        STDERR.puts "Error launching"
        exit 2
    end

    cmd = nil
    keyThread = Thread.new do
        while runner.running?
            print "\r(r)eload, (R)estart, or (q)uit: "
            cmd = nil
            cmd = IO.console.gets.chomp
            case cmd
            when "r"
                print "Reloading\n"
                runner.kill("USR1")
            when "R"
                print "Restarting\n"
                runner.kill("USR2")
            when "q"
                print "Quitting\n"
                runner.kill("INT")
                if !runner.wait(1)
                    runner.kill("TERM")
                    if !runner.wait(1)
                        runner.kill("KILL")
                    end
                end
                rebuild(runner)
                exit
            end
        end
    end

    while runner.running?
        keyThread.join(1)
    end
    if !runner.status.success? && ["r", "R"].include?(cmd)
        if runner.status.signaled? && runner.status.to_i == 11 # SIGSEGV
            STDERR.puts "Process segfaulted after hot #{cmd == "r" ? "reload" : "restart"}, trying to repair"
            rebuild(runner)
        end
        STDERR.puts "Runner exited with #{runner.status.to_s}"
    elsif !cmd
        rebuild(runner)
        exit 0
    end
end

while true
    main()
end
