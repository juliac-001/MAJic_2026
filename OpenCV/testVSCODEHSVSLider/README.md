This is just a sample example of an OpenCV project using YOLO in C++
using VSCode.

Some things to note:
  <ul>
    <li>Put all your source files in <i>src</i> subfolder (.c, .cpp, .h).  No matter they will all compile and link together.</li>
    <li>Your program will be compiled and put into the <i>bin</i> subfolder.  </li>
    <li>By default the executable will be named <i>main.out</i>.  This can be changed but you need to change all instances of main.out in both <i>launch.json</i> and <i>tasks.json</i> (there is only one instance in each file)</li>
   <li>To build, select the menu options: <i>Terminal->Run Build Task</i> or simply use the keyboard shortcut <b>Ctrl+Shift+B</b></li>
    <li><b>Always</b> perform a build before running or debugging.  Otherwise you might be debugging an old version and the code will not match what is in the executable.</li>
    <li>By default the debugger will not break in the main function like VS does.  To change this behavior set the "stopAtEntry" option in launch.json to <b>true</b>. You can also simply add a breakpoint to the first line of code</li>
   </ul>