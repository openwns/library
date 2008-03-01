###############################################################################
# This file is part of openWNS (open Wireless Network Simulator)
# _____________________________________________________________________________
#
# Copyright (C) 2004-2007
# Chair of Communication Networks (ComNets)
# Kopernikusstr. 16, D-52074 Aachen, Germany
# phone: ++49-241-80-27910,
# fax: ++49-241-80-22242
# email: info@openwns.org
# www: http://www.openwns.org
# _____________________________________________________________________________
#
# openWNS is free software; you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License version 2 as published by the
# Free Software Foundation;
#
# openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

import types
import sys

def attrsetter(it, attrs):
    for key, value in attrs.items():
        setattr(it, key, value)

class OnlyImmutableAttributes(object):
    def __new__(cls, *args, **kw):
        for name in dir(cls):
            cls.__test(cls, name)

        return super(OnlyImmutableAttributes, cls).__new__(cls, *args, **kw)

    @staticmethod
    def __test(cls, name):
        if name.startswith('__'):
            return

        value = getattr(cls, name)
        if not isinstance(value, (int, bool, float, str, tuple, property,
                          types.ClassType, types.NoneType,
                          types.FunctionType, types.MethodType)):
            print "WARNING: class %s from module %s instantiated," \
                  % (cls.__name__, cls.__module__)
            print "         although attribute %s is of mutable type %s." \
                  % (name, type(value))


class Frozen(OnlyImmutableAttributes):
    def __setattr__(self, name, value):
	raise AttributeError("No write access to the frozen object %s" % self)

def Plugin(name):
    class Strategy(Frozen):
        __plugin__ = name

    return Strategy

class View(dict):
    """Dictionary implementation serving as globals() for wns::pyconfig::View"""

    def __init__(self, obj):
        super(View, self).__init__()
        self.obj = obj

    def __contains__(self, key):
        return hasattr(self.obj, key)

    def __getitem__(self, key):
        try:
            return getattr(self.obj, key)
        except:
            if key == '__id__':
                return id(self.obj)
            if key == '__visible__':
                return self.asString(4)
            if key == '__stringRepresentation__':
                return self.asString(4)
	    try:
                return getattr(__builtins__, key)
            except:
                return __builtins__[key]

    def asString(self, indentSize):
        result = ' '*indentSize
        result += 'Above entry is of type: ' + str(self.obj) + '\n'
        result += ' '*indentSize
        for it in dir(self.obj):
            if not it.startswith('_'):
                result += it + ' = ' + str(eval('obj.'+it, {'obj':self.obj})) + '\n'
                result += ' '*indentSize
        return result[:-indentSize].rstrip("\n")

class IAttributeHandler(property):
    # we are a property ourselfs :)
    def __init__(self, getter=None, setter=None, deller=None, doc=None):
        super(IAttributeHandler, self).__init__(getter, setter, deller, doc)

    """ MetaConf uses these to inject the necessary information """
    def setAttributeName(self, attrName):
        self._attributeName = attrName

    def setClassName(self, className):
        self._className = className

class MetaConf(type):

    def __new__(cls, className, bases, dct):
        # cls == MetaConf
        if not dct.has_key("__attributes__"):
            dct["__attributes__"] = []
        for attributeName, attribute in dct.items():
            if isinstance(attribute, IAttributeHandler):
                # inject the name of the attribute and the name of the class
                attribute.setAttributeName(attributeName)
                attribute.setClassName(className)
                dct["__attributes__"].append(attributeName)
        return type.__new__(cls, className, bases, dct)

    def __init__(cls, className, bases, dct):
        super(MetaConf, cls).__init__(className, bases, dct)


class Config(object):

    class NoSuchAttribute(Exception):
        """ This exception is thrown if an attribute is accessed that is
        not available """

        def __init__(self, obj, name):
            """ The constructor takes the object where the attribute was
            missing and the name of the attribute. It prints all allowed attributes"""

            cls = obj.__class__
            error = "\nAttribute '" + name + "' not allowed in '" + cls.__name__ + "'"
            error += "\nAllowed attributes are:"
            for allowedAttribute in obj.__attributes__:
                error += "\n  " + allowedAttribute
            super(Config.NoSuchAttribute, self).__init__(error)

    __metaclass__ = MetaConf

    # this is used by the params to store the actual instances
    __attributeValues = None

    def __setattr__(self, attr, value):
        try:
            getattr(self, attr) # raises an exception in case attr is not known
        except AttributeError:
            raise Config.NoSuchAttribute(self, attr)
        # we didn't find it in the alloweed attributes list
        object.__setattr__(self, attr, value)

    def __new__(cls):
        obj = object.__new__(cls)
        obj.__attributeValues = {}
        return obj

    @classmethod
    def getAttributeHandler(cls, name):
        return cls.__dict__[name]

    def isAttributeSet(self, name):
        return self.__attributeValues.has_key(name)

    def getAttribute(self, name):
        return self.__attributeValues[name]

    def setAttribute(self, name, value):
        self.__attributeValues[name] = value


class Parameter(IAttributeHandler):
    class NoValueSet(Exception):
        def __init__(self):
            super(Parameter.NoValueSet, self).__init__("No value is set!")

    def __init__(self, doc, *hooks):
        super(Parameter, self).__init__(self.getter, self.setter)

        self.hooks = hooks
        # hooks can implement, if they implement it, they will be
        # asked to provide something. they all get: the
        # attributeValue, the AtrributeHandler and the object where to
        # set the attribute. Available hooks are:
        # onAttributeGetButNotSet()
        # beforeAttributeSet()
        # afterAttributeGet()
        # getDocString()

        # convert to string, __doc__ MUST be a string
        self.__doc__ = str(doc)

        # append additional documentation contributed by hooks
        for hook in self.hooks:
            if hasattr(hook, "getDocString"):
                self.__doc__ += "\n\n" + str(hook.getDocString())

        for hook in self.hooks:
            if hasattr(hook, "getDocField"):
                self.__doc__ += "\n\n" + str(hook.getDocField())

    def setter(self, obj, attributeValue):
        """ The setter takes exactly two parameters: the object for
        which the parameter should be set and the value to be set """

        assert isinstance(obj, Config)
        for hook in self.hooks:
            if hasattr(hook, "beforeAttributeSet"):
                attributeValue = hook.beforeAttributeSet(attributeValue, self, obj)
        obj.setAttribute(self._attributeName, attributeValue)

    def getter(self, obj):
        """ The getter takes exactly one parameters: the object from
        which the parameter should be fetched"""

        assert isinstance(obj, Config)
        if obj.isAttributeSet(self._attributeName):
            attributeValue = obj.getAttribute(self._attributeName)
            for hook in self.hooks:
                if hasattr(hook, "afterAttributeGet"):
                    attributeValue = hook.afterAttributeGet(attributeValue, self, obj)
            return attributeValue
        else:
            for hook in self.hooks:
                # the first hook that matches is fine!
                if hasattr(hook, "onAttributeGetButNotSet"):
                    attributeValue = hook.onAttributeGetButNotSet(self, obj)
                    # call afterAttributeGet hooks
                    for hook in self.hooks:
                        if hasattr(hook, "afterAttributeGet"):
                            attributeValue = hook.afterAttributeGet(attributeValue, self, obj)
                    return attributeValue
            raise Parameter.NoValueSet()



class RangeCheck(object):
    class Error(Exception):
        def __init__(self, cls, name, start, stop, value, doc):
            error =  "\nRangeCheck.Error for for '%s.%s'" % (cls, name)
            error += "\n  range: [%f, %f]" % (start, stop)
            error += "\n  value: '%f'" % (value)
            error += "\n    doc: %s" % (doc)
            super(RangeCheck.Error, self).__init__(error)

    def __init__(self, start, stop):
        self.start = start
        self.stop = stop
        self.afterAttributeGet = self.beforeAttributeSet

    def getDocString(self):
        return "B{Range}: [%s .. %s]" % (str(self.start), str(self.stop))

    def beforeAttributeSet(self, attributeValue, attributeHandler, obj):
        if not (attributeValue >= self.start and attributeValue <= self.stop):
            raise RangeCheck.Error(
                attributeHandler._className,
                attributeHandler._attributeName,
                self.start,
                self.stop,
                attributeValue,
                attributeHandler.__doc__)
        return attributeValue


class TypeCheck(object):
    class Error(Exception):
        def __init__(self, cls, name, paramType, value, doc):
            error =  "\nTypeCheck.Error for '%s.%s'" % (cls, name)
            error += "\n  expected: '%s'" % (paramType.__name__)
            error += "\n       got: '%s'" % (type(value).__name__)
            error += "\n       doc: %s" % (doc)
            super(TypeCheck.Error, self).__init__(error)

    def __init__(self, theType):
        self.theType = theType
        self.afterAttributeGet = self.beforeAttributeSet

    def getDocField(self):
        return "@type: L{%s}" % self.theType.__name__

    def beforeAttributeSet(self, attributeValue, attributeHandler, obj):
        if not isinstance(attributeValue, self.theType):
            raise TypeCheck.Error(
                attributeHandler._className,
                attributeHandler._attributeName,
                self.theType,
                attributeValue,
                attributeHandler.__doc__)
        return attributeValue

class Default(object):
    def __init__(self, default):
        self.value = default

    def getDocString(self):
        return "B{Default:} %s" % str(self.value)

    def onAttributeGetButNotSet(self, attributeHandler, obj):
        return self.value

class TypedParameter(Parameter):

    def __init__(self, doc, type, default):
        self.default = Default(default)
        super(TypedParameter, self).__init__(doc, TypeCheck(type), self.default)

    def setDefault(self, default):
        self.default.value = default

class Int(TypedParameter):
    def __init__(self, doc, default):
        super(Int, self).__init__(doc, int, default)

class String(TypedParameter):
    def __init__(self, doc, default):
        super(String, self).__init__(doc, str, default)

class Float(TypedParameter):
    def __init__(self, doc, default):
        super(Float, self).__init__(doc, float, default)

class PyConfigTest:
    foo = 42
    def __init__(self):
        """test wns::pyconfig::Parser::fromClass"""
        pass

class PyConfigTestWithArg:
    foo = 42
    def __init__(self, n):
        """test wns::pyconfig::Parser::fromClass"""
        self.foo = n

def main():
    class A(Frozen):
	x = 3

    class B(A):
	y = 42

    a = A()
    b = B()
    print a.x, b.y

    try:
        b.z = 32
    except:
	pass
    else:
	raise "Expected exception"

    try:
        a.x = 4
    except:
        pass
    else:
        raise "Expected exception"

if __name__ == '__main__':
    main()
