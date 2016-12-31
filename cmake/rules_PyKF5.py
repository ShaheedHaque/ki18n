#
# Copyright 2016 by Shaheed Haque (srhaque@theiet.org)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301  USA.
#
"""
SIP binding customisation for PyKF5.KCoreAddons. This modules describes:

    * The SIP file generator rules.
"""

import os, sys

import rules_engine
sys.path.append(os.path.dirname(os.path.dirname(rules_engine.__file__)))
import Qt5Ruleset

from clang.cindex import CursorKind

def i18n_ellipsis(container, function, sip, matcher):
    if container.kind == CursorKind.TRANSLATION_UNIT:
        if len(sip["template_parameters"]) == 0:
            sip["decl"] += ["..."]
        else:
            sip["name"] = ""

def i18np_ellipsis(container, function, sip, matcher):
    if container.kind == CursorKind.TRANSLATION_UNIT:
        if len(sip["template_parameters"]) == 1:
            sip["decl"][-1] = "..."
            sip["template_parameters"] = ""
        else:
            sip["name"] = ""

def local_function_rules():
    return [
        ["KLocalizedString", "subs", ".*", ".*", ".*unsigned int.*", rules_engine.function_discard],
        ["KLocalizedString", "subs", ".*", ".*", ".*long.*", rules_engine.function_discard],
        ["KLocalizedString", "subs", ".*", ".*", ".*unsigned long.*", rules_engine.function_discard],
        ["KLocalizedString", "subs", ".*", ".*", ".*unsigned long long.*", rules_engine.function_discard],
        ["KLocalizedString", "subs", ".*", ".*", ".*QChar.*", rules_engine.function_discard],
        ["Kuit", "setupForDomain", ".*", ".*", ".*", rules_engine.function_discard],
        ["KuitSetup", "setTagPattern", ".*", ".*", ".*", rules_engine.function_discard],

        [".*", "i18n", ".*", ".*", ".*", i18n_ellipsis],
        [".*", "i18nc", ".*", ".*", ".*", i18n_ellipsis],
        [".*", "i18np", ".*", ".*", ".*", i18np_ellipsis],
        [".*", "i18ncp", ".*", ".*", ".*", i18np_ellipsis],
    ]

def _klocalizedstring_add_template_code(filename, sip, entry):
    sip["code"] = """
%ModuleCode
QString klocalizedstring_i18n_template(KLocalizedString base, PyObject *list,int *sipIsErr) {
    KLocalizedString result = base;
    QString *arg;
    long long_arg;
    double double_arg;
    int iserr = 0;

    for (int i=0; i < PyTuple_Size(list); i++) {
        PyObject *pyarg = PyTuple_GET_ITEM (list, i);
#if PY_MAJOR_VERSION >= 3
        if (PyLong_Check(pyarg)) {
            long_arg = PyLong_AsLong(pyarg);
#else
        if (PyInt_Check(pyarg)) {
            long_arg = PyInt_AsLong(pyarg);
#endif
            if (long_arg==-1 && PyErr_Occurred()) {
                *sipIsErr = 1;
                return QString();
            }
            result = result.subs(long_arg);

#if PY_MAJOR_VERSION >= 3
        } else if (PyNumber_Check(pyarg)) {
            PyObject *long_py = PyNumber_Long(pyarg);
            long_arg = PyLong_AsLong(long_py);
            Py_DECREF(long_py);
#else
        } else if (PyLong_Check(pyarg)) {
            long_arg = PyLong_AsLong(pyarg);
#endif
            if (long_arg==-1 && PyErr_Occurred()) {
                *sipIsErr = 1;
                return QString();
            }
            result = result.subs(long_arg);

        } else if (PyFloat_Check(pyarg)) {
            double_arg = PyFloat_AsDouble(pyarg);
            result = result.subs(double_arg);

        } else {
          int state = 0;
          arg = (QString *)sipForceConvertToType(pyarg, sipType_QString, NULL, SIP_NOT_NONE, &state, &iserr);
          if (iserr) {
              *sipIsErr = 1;
              return QString();
          }

          result = result.subs(*arg);
          sipReleaseType(arg,sipType_QString,state);
          arg = 0;
          }
    }

    return result.toString();
}
%End\n
"""

class RuleSet(Qt5Ruleset.RuleSet):
    """
    SIP file generator rules. This is a set of (short, non-public) functions
    and regular expression-based matching rules.
    """
    def __init__(self, includes):
        Qt5Ruleset.RuleSet.__init__(self, includes)
        self._fn_db = rules_engine.FunctionRuleDb(lambda: local_function_rules() + Qt5Ruleset.function_rules())
        self._methodcode = rules_engine.MethodCodeDb({
            "klocalizedstring.h": {
                "i18n":
                {
                    "code":
                    """
                    %MethodCode
                        QString result = klocalizedstring_i18n_template(ki18n(a0),a1,&sipIsErr);
                        if (!sipIsErr) {
                            sipRes = new QString(result);
                        }
                    %End
                    """
                },
                "i18nc":
                {
                    "code":
                    """
                    %MethodCode
                        QString result = klocalizedstring_i18n_template(ki18nc(a0,a1),a2,&sipIsErr);
                        if (!sipIsErr) {
                            sipRes = new QString(result);
                        }
                    %End
                    """
                },
                "i18np":
                {
                    "code":
                    """
                    %MethodCode
                        QString result = klocalizedstring_i18n_template(ki18np(a0,a1),a2,&sipIsErr);
                        if (!sipIsErr) {
                            sipRes = new QString(result);
                        }
                    %End
                    """
                },
                "i18ncp":
                {
                    "code":
                    """
                    %MethodCode
                        QString result = klocalizedstring_i18n_template(ki18ncp(a0,a1,a2),a3,&sipIsErr);
                        if (!sipIsErr) {
                            sipRes = new QString(result);
                        }
                    %End
                    """
                },
            }
        })

        self._modulecode = rules_engine.ModuleCodeDb({
            "klocalizedstring.h":
            {
                "code": _klocalizedstring_add_template_code,
            },
            })
