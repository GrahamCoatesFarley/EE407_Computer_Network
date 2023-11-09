# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('dvhop', ['core'])
    module.source = [
        'model/dvhop.cc',
        'model/dvhop-packet.cc',
        'model/distance-table.cc',
        'helper/dvhop-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('dvhop')
    module_test.source = [
        'test/dvhop-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'dvhop'
    headers.source = [
        'model/dvhop.h',
        'model/dvhop-packet.h',
        'model/distance-table.h',
        'helper/dvhop-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    #Uncomment the next line to enable the python bindings
    #bld.ns3_python_bindings()

