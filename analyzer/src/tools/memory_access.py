import pathlib

import trace_file


class MemoryAccess:

    def __init__(self, file_path: pathlib.Path, mode: int):
        self._file_path = file_path
        self._file = None
        self._mode = mode


    def __enter__(self) -> "MemoryAccess":
        self._file = open(self._file_path, 'w', encoding='utf-8')
        return self


    def __exit__(self, *args):
        self._file.close()


    def process_instruction(self, instruction: trace_file.Instruction) -> None:
        memory_address = self._compute_memory_address(instruction)
        if memory_address is not None:
            disassembly = f'{instruction.mnemonic} {instruction.operands}'
            self._file.write(f"0x{instruction.address :0{self._mode // 4}X} | {disassembly :<50} | {memory_address :0{self._mode // 4}X}\n")


    def _compute_memory_address(self, instruction: trace_file.Instruction) -> None | int:
        start = instruction.operands.find('[')
        end = instruction.operands.find(']')
        if start == -1 or end == -1 or instruction.mnemonic == 'lea':
            return None

        locals = {}
        source = ''
        source += '\n'.join(f'{name} = {value}' for name, value in instruction.registers.items())
        source += f'\nmemory_address = {instruction.operands[start + 1:end]}'
        exec(source, locals=locals)

        return locals['memory_address'] & int('F' * (self._mode // 4), 16)
