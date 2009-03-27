import openwns.toolsupport
import openwns.simulator

class OpenWNSVisitor(openwns.toolsupport.IPyTreeVisitor):

    def renderShortText(self, o):
        return o.__class__.__name__

    def renderLongText(self, o):
        import os.path
        result =  "Maximum Simulation Time : %s\n" % str(o.maxSimTime)
        result += "Write Status every : %s s (realtime)\n" % str(o.statusWriteInterval)
        result += "Write Probes every : %s s (realtime)\n" % str(o.probesWriteInterval)
        result += "Output directory : %s\n" % os.path.abspath(o.outputDir)
        result += "Status filename : %s\n" % o.statusFileName

        result += "\n\n"
        result += openwns.toolsupport.PyTreeVisitorFactory.getVisitor(o.simulationModel).renderLongText(o.simulationModel)

        return result

class EnvironmentVisitor(openwns.toolsupport.IPyTreeVisitor):

    def renderShortText(self, o):
        return o.__class__.__name

    def renderLongText(self, o):
        return "Long description of Environment"

openwns.toolsupport.PyTreeVisitorFactory.registerVisitor(openwns.simulator.OpenWNS, OpenWNSVisitor())
openwns.toolsupport.PyTreeVisitorFactory.registerVisitor(openwns.simulator.Environment, EnvironmentVisitor())
