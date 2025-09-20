import pathlib
from collections import Counter
from tkinter import filedialog

from core import trace_file


def main() -> None:
    trace_file_name = filedialog.askopenfilename()
    output_directory = filedialog.askdirectory()
    mode = int(input("Mode (32/64): "))

    mnemonics = Counter()
    for instruction in trace_file.parse(trace_file_name, mode):
        mnemonics[instruction.mnemonic] += 1
    
    output_file_name = pathlib.Path(output_directory) / 'mnemonic_statistics.txt'
    with open(output_file_name, 'w', encoding='utf-8') as file:
        for mnemonic, count in sorted(mnemonics.items()):
            file.write(f"{mnemonic :<10} {count}\n")


if __name__ == '__main__':
    main()
