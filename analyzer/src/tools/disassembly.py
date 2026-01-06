import pathlib

import trace_file


class Disassembly:

    def __init__(self, file_path: pathlib.Path, mode: int):
        self._file_path = file_path
        self._file = None
        self._mode = mode


    def __enter__(self) -> "Disassembly":
        self._file = open(self._file_path, 'w', encoding='utf-8')
        return self


    def __exit__(self, *args):
        self._file.close()


    def process_instruction(self, instruction: trace_file.Instruction) -> None:
        address = f'0x{instruction.address :x}:'
        disassembly = f'{instruction.mnemonic} {instruction.operands}'
        registers = [f'{name}=0x{value :0{self._mode // 4}X}' for name, value in instruction.registers.items()]

        self._file.write(f"{address :<{self._mode // 2}} {disassembly :<50} ; {' '.join(registers)}\n")
