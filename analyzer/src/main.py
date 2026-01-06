import pathlib
from tkinter import filedialog

import trace_file
from tools.instruction_statistics import InstructionStatistics
from tools.disassembly import Disassembly
from tools.memory_access import MemoryAccess


def main() -> None:
    trace_file_path = pathlib.Path(filedialog.askopenfilename())
    output_directory_path = pathlib.Path(filedialog.askdirectory())
    mode = int(input("Mode (32/64): "))

    instruction_statistics = InstructionStatistics(output_directory_path / 'instruction_statistics.txt')
    disassembly = Disassembly(output_directory_path / 'disassembly.txt', mode)
    memory_access = MemoryAccess(output_directory_path / 'memory_access.txt', mode)

    with disassembly, memory_access:
        for instruction in trace_file.parse_trace_file(trace_file_path, mode):
            instruction_statistics.process_instruction(instruction)
            disassembly.process_instruction(instruction)
            memory_access.process_instruction(instruction)

    instruction_statistics.save()


if __name__ == '__main__':
    main()
