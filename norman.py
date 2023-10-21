#!/usr/bin/env python3

import os
import subprocess
import shutil
import argparse
import logging
import hashlib  # Added hashlib for SHA sum calculation


def calculate_hash(file_path):
    hasher = hashlib.sha256()
    with open(file_path, 'rb') as f:
        while True:
            chunk = f.read(65536)  # Read in 64k chunks
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="")

    parser.add_argument('TARGET',
                       metavar='TARGET',
                       type=str,
                       help='the path to c program for which the product has to be constructed')
    parser.add_argument('filter',
                       metavar='filter',
                       type=str,
                       help='The path to the filter.json file containing instructions on which functions to process.')
    args = parser.parse_args()

    stage_command = construct_stage_command(tool_bin_path,"NormaliseExprTransform",stage_output_filename,src,output_dir,output_path,orig_file,run_one,filter_path)
    current_hash = None
    while True:
        subprocess.run(stage_command, stdout=None, stderr=None, check=True)
        output_file_path = os.path.join(output_dir_name, stage_output_filename)
        next_hash = calculate_hash(output_file_path)  # Calculate current SHA sum
        if current_hash == next_hash:
            break
        current_hash = next_hash  # Update previous SHA sum
