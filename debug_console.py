import os
import sys
import code

BANNER = "--- FromSoftSaveManger debug console ---"


class DebugConsole(code.InteractiveConsole):
    def interact(self, banner=None, exitmsg=None):
        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        try:
            sys.ps2
        except AttributeError:
            sys.ps2 = "... "

        self.write("%s\n" % str(banner))
        more = 0
        while 1:
            try:
                if more:
                    prompt = sys.ps2
                else:
                    prompt = sys.ps1
                try:
                    line = self.raw_input(prompt)
                except EOFError:
                    self.write("\n")
                    break
                else:
                    more = self.push(line)
            except KeyboardInterrupt:
                self.write("\n> KeyboardInterrupt <\n")
                self.resetbuffer()
                break

        self.write("Exiting now...\n")


def exit():
    sys.exit(0)


def run_interactive():
    console = DebugConsole({
        "exit": exit,
    })
    console.interact(BANNER, None)


def run_script(args):
    # Add parent dir to sys path
    filepath = args[0]
    sys.path.insert(0, os.path.dirname(filepath))

    # Read content and execute
    with open(filepath, "r") as stream:
        content = stream.read()

    script_globals = dict(globals())
    script_globals["__file__"] = filepath
    exec(compile(content, filepath, "exec"), script_globals)


def main():
    args = list(sys.argv)
    args.pop(0)
    if args:
        run_script(args)
    else:
        run_interactive()


if __name__ == "__main__":
    main()
