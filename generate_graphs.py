import subprocess
import sys
import pandas as pd
from openpyxl import load_workbook
import openpyxl
from openpyxl.utils.dataframe import dataframe_to_rows
from openpyxl.drawing.image import Image
from openpyxl.chart import BarChart, Reference
from openpyxl.styles import Font


def run_code():
    build_result = subprocess.run(["make"], text=True, capture_output=True)
    print(type(build_result.stdout))
    output = None
    if (build_result.returncode == 0):
        output = build_result.stdout
    return output

def parse_output(output):
    if "Distributions (KB)" in output:
        start_index = output.index("Distributions (KB)") + len("Distributions (KB)")
        output = output[start_index:].strip()

    data = {}
    for line in output.split("\n"):
        if "->" in line:
            key, value = line.split(":")
            category = key.strip().split(" ")[0]
            memcoh = key.strip().split(" ")[1]
            if (category not in data.keys()):
                data[category] = {}
            data[category][memcoh] = float(value.strip())
    return data

def create_table(data, workload_name):
    table_data = []
    for category, values in data.items():
        memory_kb = values["Memory"]
        coherence_kb = values["Coherence"]
        total_kb = memory_kb + coherence_kb

        memory_percent = 100 * memory_kb / total_kb if total_kb > 0 else 0
        coherence_percent = 100*coherence_kb / total_kb if total_kb > 0 else 0

        table_data.append({
            "Workload": workload_name,
            "Category": category,
            "Memory KB": memory_kb,
            "Coherence KB": coherence_kb,
            "Memory %": memory_percent,
            "Coherence %": coherence_percent
        })

        df = pd.DataFrame(table_data)
    return df

def append_to_excel_with_graph(df, sheet_name, header, output_file="simulation_results.xlsx"):
    try:
        # Load workbook and check if sheet exists
        book = load_workbook(output_file)
    except FileNotFoundError:
        # If the file or sheet doesn't exist, create new
        book = openpyxl.Workbook()

    if sheet_name in book.sheetnames:
        sheet = book[sheet_name]
    else:
        sheet = book.create_sheet(sheet_name)

    # Find the last row and append new data with a 2-row gap
    last_row = sheet.max_row
    start_row = last_row + 3

    # Insert header above the new table
    header_cell = sheet.cell(row=start_row, column=1)
    header_cell.value = header
    header_cell.font = Font(bold=True)

    # Append the data table
    for r_idx, row in enumerate(dataframe_to_rows(df, index=False, header=True), start=start_row + 1):
        for c_idx, value in enumerate(row, start=1):
            sheet.cell(row=r_idx, column=c_idx, value=value)

    # Save the workbook
    book.save(output_file)


def main():
    # output = run_code()
    # if output == None:
    #     return

    with open(sys.argv[3], "r") as infile:
        output = infile.read()
        data = parse_output(output)
        df = create_table(data, f"cluster{sys.argv[2]}")

        output_file = "simulation_results.xlsx"
        sheet_name = f"cluster{sys.argv[2]}"
        header = sys.argv[1]

        append_to_excel_with_graph(df, sheet_name, header, output_file)

if __name__ == '__main__':
    main()
    