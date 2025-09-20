import pathlib
from tkinter import filedialog

from core import trace_file


def main() -> None:
    trace_file_name = filedialog.askopenfilename()
    output_directory = filedialog.askdirectory()

    mnemonics = {}
    for instruction in trace_file.parse(trace_file_name):
        mnemonic = instruction.disassembly.mnemonic
        if mnemonic not in mnemonics:
            mnemonics[mnemonic] = 0
        mnemonics[mnemonic] += 1
        
    with open(pathlib.Path(output_directory) / 'mnemonic_statistics.txt', 'w', encoding='utf-8') as file:
        for mnemonic, count in sorted(mnemonics.items()):
            file.write(f"{mnemonic :<10} {count}\n")


if __name__ == '__main__':
    main()
