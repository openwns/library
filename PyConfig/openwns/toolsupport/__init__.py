import openwns.interface

class PyTreeVisitorFactory:

    visitors = {}

    @staticmethod
    def registerVisitor(cls, visitor):
        PyTreeVisitorFactory.visitors[cls] = visitor

    @staticmethod
    def getVisitor(obj):
        import openwns.toolsupport.pytreevisitors
        for cls in type(obj).mro():
            try:
                v = PyTreeVisitorFactory.visitors[cls]
                return v
            except KeyError:
                pass
        return EmptyPyTreeVisitor()

class IPyTreeVisitor(openwns.interface.Interface):

    def renderShortText(self, objectToRender):
        pass

    def renderLongText(self, objectToRender):
        pass

class EmptyPyTreeVisitor(IPyTreeVisitor):

    def renderShortText(self, objectToRender):
        return ""

    def renderLongText(self, objectToRender):
        return ""
