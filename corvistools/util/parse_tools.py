"""
Parses stderr output from the filter to identify common occurences
"""

import re
from collections import defaultdict

STDERR_RE = [
    # (regex, accumulation_key)
    # An accumulation key of none will consume the line, but not
    # accumulate anything
    ("^Warning: received packets out of order", "packet_order_warning"),
    ("^Velocity (.*) exceeds", "velocity_error"),
    ("^Velocity exceeds", "velocity_error"),
    ("^High velocity (.*) warning", "velocity_warning"),
    ("^High velocity warning", "velocity_warning"),
    ("^High acceleration (.*) warning", "acceleration_warning"),
    ("^Acceleration exceeds max bound", "acceleration_max_bound"),
    ("^High angular velocity warning", "angular_velocity_warning"),
    ("^detector failure: only (\d+)", "detector_failure"),
    ("^Inertial did not converge (.*), (.*)", "inertial_warning"),
    ("^full filter reset", "full_filter_reset"),
    ("^measurement starting", None),
    ("^Launching plugins", None),
    ("^Launched a plugin", None),
    ("^All plugins launched", None),
    ("^priority", None),
    ]

STDOUT_MEASURE_RE = [
    # (regex, accumulation_key)
    # An accumulation key of none will consume the line, but not
    # accumulate anything
    ("Straight line length \(cm\): (.*)", "L"),
    ("Total path length \(cm\): (.*)", "PL"),
    ("^No packets received recently", "no_end_packet"),
    ("Configuration name", None),
    ("Filename", None),
    ]

def parse_stream(lines, regexes):
    parsed = defaultdict(dict)
    unconsumed = []
    for line in lines.splitlines():
        matched = False
        for (regex, key) in regexes:
            M = re.search(regex, line)
            if M:
                if key:
                    if key in parsed:
                        parsed[key]["count"] += 1
                        if len(M.groups()) > 0:
                            parsed[key]["data"].append(M.groups())
                    else:
                        parsed[key]["count"] = 1
                        if len(M.groups()) > 0:
                            parsed[key]["data"] = [M.groups()]
                matched = True
                break

        if not matched: 
            unconsumed.append(line)


    parsed["unconsumed"] = unconsumed
    return parsed


def parse_stderr(stderr):
    return parse_stream(stderr, STDERR_RE)


def parse_measure_stdout(stdout):
    return parse_stream(stdout, STDOUT_MEASURE_RE)
