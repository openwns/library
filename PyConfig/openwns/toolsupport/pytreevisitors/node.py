import openwns.toolsupport
import openwns.node

class NodeSimulationModelVisitor(openwns.toolsupport.IPyTreeVisitor):

    def renderShortText(self, o):
        return o.__class__.__module__ + "." + o.__class__.__name__

    def renderLongText(self, o):
        result = "SimulationModel : %s\n" % self.renderShortText(o)
        result += "Number of Nodes : %d\n" % len(o.nodes)
        result += "Nodes : %s\n" % str([n.name for n in o.nodes])
        return result


class NodeVisitor(openwns.toolsupport.IPyTreeVisitor):

    def renderShortText(self, o):
        return o.name + " (" + o.__class__.__module__ + "." + o.__class__.__name__ + ")"

    def renderLongText(self, o):
        result =  "Node name : %s\n" % o.name
        result += "Number of Components : %d\n" % len(o.components)
        result += "Components : %s\n" % str([n.name for n in o.components])
        return result

class ComponentVisitor(openwns.toolsupport.IPyTreeVisitor):

    def renderShortText(self, o):
        return o.name + " (" + o.__class__.__module__ + "." + o.__class__.__name__ + ")"

    def renderLongText(self, o):
        result =  "Component name : %s\n" % o.name
        result += "Parent Node : %s\n" % o.node.name
        return result

openwns.toolsupport.PyTreeVisitorFactory.registerVisitor(openwns.node.NodeSimulationModel, NodeSimulationModelVisitor())
openwns.toolsupport.PyTreeVisitorFactory.registerVisitor(openwns.node.Node, NodeVisitor())
openwns.toolsupport.PyTreeVisitorFactory.registerVisitor(openwns.node.Component, ComponentVisitor())
