#!/usr/bin/env python3

# This program will submit a solution to the assignment by inserting a
# file into a directory that has restricted access.

# This program should only be used by students who cannot submit
# solutions via Autolab (e.g., those who are still on the waitlist).

import shutil
import sys
import getopt
import glob

submit_directory = "/afs/cs.cmu.edu/academic/class/15418-f26/public/asst/asst1-handin-f26"

def usage(name):
    print(f"Usage: {name} -u USER [-h]")
    print("       -u USER   Specify Andrew ID of student")
    print("       -h        Print this message")
    print("NOTE: this script is only for students on the waitlist.  Registered students should use Autolab instead.")

def submit(user_id):
    version = 1
    prefix = f"{submit_directory}/handin-{user_id}-v"
    template = f"{prefix}*.tgz"
    file_list = glob.glob(template)
    for fname in file_list:
        pos = len(prefix)
        digits = ""
        try:
            while fname[pos].isdigit():
                digits += fname[pos]
                pos += 1
        except IndexError:
            pass
        if digits:
            version = max(version, int(digits) + 1)
    dest_name = f"{prefix}{version}.tgz"
    try:
        shutil.copyfile("handin.tgz", dest_name)
    except Exception as e:
        print(f"FAILED to copy handin.tgz to destination '{dest_name}'.  ({e})")
        sys.exit(1)
    print(f"SUCCESSFULLY copied handin.tgz to destination '{dest_name}'.")

def run(name, args):
    user_id = ""
    try:
        optlist, args = getopt.getopt(args, "hu:")
    except getopt.GetoptError as err:
        print(err)
        usage(name)
        sys.exit(1)

    for opt, val in optlist:
        if opt == '-h':
            usage(name)
            sys.exit(0)
        elif opt == '-u':
            user_id = val
        else:
            print(f"Unknown option '{opt}'")
            usage(name)
            sys.exit(0)

    if not user_id:
        print("You must provide your Andrew ID")
        usage(name)
        sys.exit(0)
    submit(user_id)

if __name__ == "__main__":
    run(sys.argv[0], sys.argv[1:])
