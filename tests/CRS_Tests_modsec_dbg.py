from ftw import ruleset, logchecker, testrunner
import datetime
import pytest
import pdb
import sys
import re
import os
import config
import time

def test_crs(ruleset, test, logchecker_obj):
    #configure modsec audit loogging & need to make sure paranoia is set to 4?
    # read the modsec conf - modsecurity.conf - set the following:
    # SecDebugLog E:\ModSecLogs\modsec_debug.log
    # SecDebugLogLevel 3

    runner = testrunner.TestRunner()
    for stage in test.stages:
        runner.run_stage(stage, logchecker_obj)
        
class FooLogChecker(logchecker.LogChecker):
    log_file = None

    def __init__(self):
        modsec_conf_file = config.modsecurity_conf_location
        with open(modsec_conf_file) as fp:
            required_conf_elements = 2
            for line in fp:
                if 'SecDebugLog ' in line and line[0] != '#':
                    self.log_file = line.split(' ')[1].rstrip()
                    required_conf_elements -= 1

                if 'SecDebugLogLevel' in line and line[0] != '#':
                    required_conf_elements -= 1

                if required_conf_elements == 0:
                    break

            if required_conf_elements != 0:
                sys.exit("Please make sure that SecDebugLog & SecDebugLogLevel in {}".format(config.modsecurity_conf_location))

    def reverse_readline(self, filename):
        print(filename)
        with open(filename, 'r') as f:
            f.seek(0, os.SEEK_END)
            position = f.tell()
            line = ''
            while position >= 0:
                f.seek(position)
                next_char = f.read(1)
                if next_char == "\n":
                    print("*********************************************************************************")
                    print("alon1: {}".format(line[::-1]))
                    yield line[::-1]
                    line = ''
                else:
                    line += next_char
                position -= 1
            print("*********************************************************************************")
            print("alon2: {}".format(line[::-1]))
            yield line[::-1]    

    def get_logs(self):
        log_date_regex = config.log_date_regex
        log_date_format = config.log_date_format
        pattern = re.compile(r'%s' % log_date_regex)
        our_logs = []
        print("reading the debug file: {}".format(self.log_file))
        for lline in self.reverse_readline(self.log_file):
            # Extract dates from each line
            match = re.match(pattern,lline)
            print ("alon, match: {}".format(match))
            if match:
                log_date = match.group(1)
                print("alon, log_data: {}".format(log_date))
                # Convert our date
                log_date = datetime.datetime.strptime(log_date[:-7], log_date_format)
                ftw_start = self.start
                print("alon, log_data / ftw_start: {}".format(log_date, ftw_start))
                # if data is bigger than start add it, otherwise abort
                if log_date >= ftw_start.replace(microsecond=0):
                    our_logs.append(lline)
                else:
                    break
        return our_logs

@pytest.fixture
def logchecker_obj():
    return FooLogChecker()
