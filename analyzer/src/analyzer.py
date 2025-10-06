import pathlib

import capstone


class Analyzer:
    
    def __init__(self, trace_file_path: str, output_directory_path: str, mode: int):
        self.trace_file_path = pathlib.Path(trace_file_path)
        self.output_directory_path = pathlib.Path(output_directory_path)
        self.mode = mode
        self.trace_file = None


    def __enter__(self):
        self.trace_file = open(self.trace_file_path, 'r', encoding='utf-8')
        return self


    def __exit__(self, exc_type, exc_value, traceback):
        self.trace_file.close()


    def parse_trace_file(self) -> None:
        md = capstone.Cs(
            arch=capstone.CS_ARCH_X86,
            mode={
                32: capstone.CS_MODE_32,
                64: capstone.CS_MODE_64,
            }[self.mode],
        )
        
        for line in self.trace_file:
            address, code, registers = line.split(' | ')
