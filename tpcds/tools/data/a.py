import random
import os


current_directory = os.getcwd()
input_file = os.listdir(current_directory)
input_file.remove('a.py')
num_lines_to_keep = 2000
for file_exist in input_file:
    with open(file_exist, 'r') as file:
        lines = file.readlines()

    filtered_lines = [line for line in lines if '||' not in line]
    total_filtered_lines = len(filtered_lines)

    if total_filtered_lines < num_lines_to_keep:
        print(f"过滤后的行数少于 {num_lines_to_keep}，将保留所有 {total_filtered_lines} 行")
        selected_lines = filtered_lines
    else:
        selected_lines = random.sample(filtered_lines, num_lines_to_keep)


    with open(file_exist, 'w') as file:
        file.writelines(selected_lines)

    print(f"已将随机选定的 {len(selected_lines)} 行写入 {file_exist}")


# import random
# import os
# import re

# current_directory = os.getcwd()
# input_file = os.listdir(current_directory)
# input_file.remove('a.py')
# num_lines_to_keep = 1000 

# for file_exist in input_file:
#     with open(file_exist, 'r') as file:
#         lines = file.readlines()

#     filtered_lines = []
#     for line in lines:
#         if '||' in line:
#             continue
#         numbers = re.findall(r'\d+', line)
#         if any(int(number) > 2500000 for number in numbers):
#             continue
#         filtered_lines.append(line)

#     total_filtered_lines = len(filtered_lines)

#     if total_filtered_lines < num_lines_to_keep:
#         print(f"过滤后的行数少于 {num_lines_to_keep}，将保留所有 {total_filtered_lines} 行")
#         selected_lines = filtered_lines
#     else:
#         selected_lines = random.sample(filtered_lines, num_lines_to_keep)

#     with open(file_exist, 'w') as file:
#         file.writelines(selected_lines)

#     print(f"已将随机选定的 {len(selected_lines)} 行写入 {file_exist}")
