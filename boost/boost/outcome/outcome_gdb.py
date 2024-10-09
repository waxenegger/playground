import gdb.printing
import os

def synthesise_gdb_value_from_string(s):
    '''For when you want to return a synthetic string from children()'''
    return gdb.Value(s + '\0').cast(gdb.lookup_type('char').pointer())

class OutcomeBasicOutcomePrinter(object):
    '''Print an outcome::basic_outcome<T> and outcome::basic_result<T>'''

    def __init__(self, val):
        self.val = val

    def children(self):
        if self.val['_state']['_status']['status_value'] & 1 == 1:
            yield ('value', self.val['_state']['_value'])
        if self.val['_state']['_status']['status_value'] & 2 == 2:
            yield ('error', self.val['_state']['_error'])
        if self.val['_state']['_status']['status_value'] & 4 == 4:
            yield ('exception', self.val['_ptr'])

    def display_hint(self):
        return None

    def to_string(self):
        if self.val['_state']['_status']['status_value'] & 54 == 54:
            return 'errored (errno, moved from) + exceptioned'
        if self.val['_state']['_status']['status_value'] & 50 == 50:
            return 'errored (errno, moved from)'
        if self.val['_state']['_status']['status_value'] & 38 == 38:
            return 'errored + exceptioned (moved from)'
        if self.val['_state']['_status']['status_value'] & 36 == 36:
            return 'exceptioned (moved from)'
        if self.val['_state']['_status']['status_value'] & 35 == 35:
            return 'errored (moved from)'
        if self.val['_state']['_status']['status_value'] & 33 == 33:
            return 'valued (moved from)'
        if self.val['_state']['_status']['status_value'] & 22 == 22:
            return 'errored (errno) + exceptioned'
        if self.val['_state']['_status']['status_value'] & 18 == 18:
            return 'errored (errno)'
        if self.val['_state']['_status']['status_value'] & 6 == 6:
            return 'errored + exceptioned'
        if self.val['_state']['_status']['status_value'] & 4 == 4:
            return 'exceptioned'
        if self.val['_state']['_status']['status_value'] & 2 == 2:
            return 'errored'
        if self.val['_state']['_status']['status_value'] & 1 == 1:
            return 'valued'
        if self.val['_state']['_status']['status_value'] & 0xff == 0:
            return 'empty'

class OutcomeCResultStatusPrinter(object):
    '''Print a C result'''

    def __init__(self, val):
        self.val = val

    def children(self):
        if self.val['flags'] & 1 == 1:
            yield ('value', self.val['value'])
        if self.val['flags'] & 2 == 2:
            yield ('error', self.val['error'])

    def display_hint(self):
        return None

    def to_string(self):
        if self.val['flags'] & 50 == 50:
            return 'errored (errno, moved from)'
        if self.val['flags'] & 35 == 35:
            return 'errored (moved from)'
        if self.val['flags'] & 33 == 33:
            return 'valued (moved from)'
        if self.val['flags'] & 18 == 18:
            return 'errored (errno)'
        if self.val['flags'] & 2 == 2:
            return 'errored'
        if self.val['flags'] & 1 == 1:
            return 'valued'
        if self.val['flags'] & 0xff == 0:
            return 'empty'


class OutcomeCStatusCodePrinter(object):
    '''Print a C status code'''

    def __init__(self, val):
        self.val = val

    def children(self):
        yield ('domain', self.val['domain'])
        yield ('value', self.val['value'])

    def display_hint(self):
        return None

    def to_string(self):
        s = str(self.val['domain'])
        if 'posix_code_domain' in s or 'generic_code_domain' in s:
            return str(self.val['value']) + ' (' + os.strerror(int(self.val['value'])) + ')'
        else:
            return self.val['value']

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('outcome_v2')
    pp.add_printer('outcome_v2::basic_result', '^(boost::)?outcome_v2[_0-9a-f]*::basic_result<.*>$', OutcomeBasicOutcomePrinter)
    pp.add_printer('outcome_v2::basic_outcome', '^(boost::)?outcome_v2[_0-9a-f]*::basic_outcome<.*>$', OutcomeBasicOutcomePrinter)
    pp.add_printer('cxx_result_status_code_*', '^cxx_result_status_code_.*$', OutcomeCResultStatusPrinter)
    pp.add_printer('cxx_status_code_*', '^cxx_status_code_.*$', OutcomeCStatusCodePrinter)
    return pp

def register_printers(obj = None):
    gdb.printing.register_pretty_printer(obj, build_pretty_printer(), replace = True)

register_printers(gdb.current_objfile())
