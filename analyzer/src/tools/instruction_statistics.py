import pathlib
import collections

import trace_file


class InstructionStatistics:

    def __init__(self, file_path: pathlib.Path):
        self._file_path = file_path
        self._mnemonics = collections.Counter()


    def process_instruction(self, instruction: trace_file.Instruction) -> None:
        self._mnemonics[instruction.mnemonic] += 1


    def save(self) -> None:
        with open(self._file_path, 'w', encoding='utf-8') as file:
            for mnemonic, count in sorted(self._mnemonics.items()):
                file.write(f"{mnemonic :<10} {count}\n")
