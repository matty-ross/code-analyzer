from tkinter import filedialog

from analyzer import Analyzer


def main() -> None:
    trace_file_path = filedialog.askopenfilename()
    output_directory_path = filedialog.askdirectory()
    mode = int(input("Mode (32/64): "))
    
    with Analyzer(trace_file_path, output_directory_path, mode) as analyzer:
        analyzer.parse_trace_file()


if __name__ == '__main__':
    main()
