import openwns.toolsupport
import openwns.probebus

class MeasurementSourceVisitor(openwns.toolsupport.IPyTreeVisitor):

    def renderShortText(self, o):
        return o.__class__.__module__ + "." + o.__class__.__name__

    def getTree(self, o, indent=""):
        result = ""
        for observer in o.observers:
            
            v = openwns.toolsupport.PyTreeVisitorFactory.getVisitor(observer)
            s = v.renderShortText(observer)

            if s == "":
                s = str(self.renderShortText(observer))

            result += indent + s + "\n"
            result += self.getTree(observer, indent.replace("+---", "    ") +"+---")
        return result

    def renderLongText(self, o):
        result = "Evaluation Tree:\n"
        result += self.getTree(o)
        return result

class ContextFilterProbeBusVisitor(openwns.toolsupport.IPyTreeVisitor):

    def renderShortText(self, o):
        return "if %s in %s" % (o.idName, str(o.idValues))

    def renderLongText(self, o):
        result =  "ContextFilterProbeBus accepts measurements if\n"
        result += "Context Entry : %s\n" % o.idName
        result += "is equal to : %s\n" % o.idValues
        return result

class StatEvalProbeBusVisitor(openwns.toolsupport.IPyTreeVisitor):

    def renderShortText(self, o):
        v = openwns.toolsupport.PyTreeVisitorFactory.getVisitor(o.statEval)
        s = v.renderShortText(o.statEval)
        if s == "":
            s = o.statEval.__class__.__module__ + "." + o.statEval.__class__.__name__ + " ("+ (o.outputFilename) + ")"
        return s

    def renderLongText(self, o):
        v = openwns.toolsupport.PyTreeVisitorFactory.getVisitor(o.statEval)
        s = v.renderLongText(o.statEval)
        return s

openwns.toolsupport.PyTreeVisitorFactory.registerVisitor(openwns.probebus.MeasurementSource, MeasurementSourceVisitor())
openwns.toolsupport.PyTreeVisitorFactory.registerVisitor(openwns.probebus.ContextFilterProbeBus, ContextFilterProbeBusVisitor())
openwns.toolsupport.PyTreeVisitorFactory.registerVisitor(openwns.probebus.StatEvalProbeBus, StatEvalProbeBusVisitor())
