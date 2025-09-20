import pathlib
from tkinter import filedialog

from core import trace_file


def main() -> None:
    trace_file_name = filedialog.askopenfilename()
    output_directory = filedialog.askdirectory()

    with open(pathlib.Path(output_directory) / 'disassembly.txt', 'w', encoding='utf-8') as file:
        for instruction in trace_file.parse(trace_file_name):
            address = f'0x{instruction.address :x}:'
            disassembly = f'{instruction.disassembly.mnemonic} {instruction.disassembly.op_str}'
            registers = [f'{name}=0x{value :08x}' for name, value in instruction.registers.items()]
            
            file.write(f"{address :<15} {disassembly :<50} ; {' '.join(registers)}\n")


if __name__ == '__main__':
    main()
