import pathlib
from tkinter import filedialog

from core import trace_file


def get_memory_address(expression: str, registers: dict[str, int], mode: int) -> int:
    loc = {}
    source = ''
    source += '; '.join(f'{name}={value}' for name, value in registers.items())
    source += f'; memory_address = {expression}'
    exec(source, locals=loc)
    return loc['memory_address'] & int('F' * (mode // 4), 16)


def main() -> None:
    trace_file_name = filedialog.askopenfilename()
    output_directory = filedialog.askdirectory()
    mode = int(input("Mode (32/64): "))

    output_file_name = pathlib.Path(output_directory) / 'memory_access.txt'
    with open(output_file_name, 'w', encoding='utf-8') as file:
        for instruction in trace_file.parse(trace_file_name, mode):
            start = instruction.operands.find('[')
            end = instruction.operands.find(']')
            if start != -1 and end != -1 and instruction.mnemonic != 'lea':
                memory_address = get_memory_address(instruction.operands[start + 1:end], instruction.registers, mode)
                file.write(f"{instruction.address :0{mode // 4}X} -> {memory_address :0{mode // 4}X}\n")


if __name__ == '__main__':
    main()
