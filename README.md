# Extended GuiObjectView v1.0
An extension to the GuiObjectView class by Will o' Wisp Games.

* Extended Features
• Sub-mesh visibility states
• Sub-mesh skinning support
• Extended camera

* Tech Blog
[Extended GuiObjectView Blog Post](http://www.willowispgames.com/tech/2016/09/15/ExtGuiObjView.html)

* Installation
<h3>Source Code:</h3>
<b>1 -</b> Backup your existing <filepath>"source\T3D\guiObjectView.h"</filepath> and <filepath>"source\T3D\guiObjectView.cpp"</filepath> files.<br>
<b>2 -</b> Replace <filepath>"source\T3D\guiObjectView.h"</filepath> and <filepath>"source\T3D\guiObjectView.cpp"</filepath> with the updated files provided.<br>
<b>3 -</b> Recompile. A successful update of the GuiObjectView class will allow the script functions to be used.<br>
<br>
<h3>Script:</h3>
An example script file has been provided, which can be initialized with the other .gui scripts in <filepath>"scripts\client\init.cs"</filepath>(As of T3D 3.9, when 4.0 hits expect filepaths for scripts to change). The example script will cause any GuiObjectView control named <filepath>'PreviewGui'</filepath> to automatically reset its camera's position when the control's <filepath>onWake()</filepath> is called. It is recommended that this script be included to promote a better understanding of the resource and how it is used from script.<br>
