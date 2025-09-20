import pathlib
from tkinter import filedialog

from core import trace_file


def main() -> None:
    trace_file_name = filedialog.askopenfilename()
    output_directory = filedialog.askdirectory()
    mode = int(input("Mode (32/64): "))

    output_file_name = pathlib.Path(output_directory) / 'disassembly.txt'
    with open(output_file_name, 'w', encoding='utf-8') as file:
        for instruction in trace_file.parse(trace_file_name, mode):
            address = f'0x{instruction.address :x}:'
            disassembly = f'{instruction.mnemonic} {instruction.operands}'
            registers = [f'{name}=0x{value :0{mode // 4}X}' for name, value in instruction.registers.items()]
            file.write(f"{address :<{mode // 2}} {disassembly :<50} ; {' '.join(registers)}\n")


if __name__ == '__main__':
    main()
