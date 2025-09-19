from tkinter import filedialog

from core import trace_file


def main() -> None:
    mnemonics = {}

    file_name = filedialog.askopenfilename()
    for instruction in trace_file.parse(file_name):
        mnemonic = instruction.disassembly.mnemonic
        if mnemonic not in mnemonics:
            mnemonics[mnemonic] = 0
        mnemonics[mnemonic] += 1
        
    print(mnemonics)


if __name__ == '__main__':
    main()
