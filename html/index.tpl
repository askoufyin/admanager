﻿<h3>Valid macro formats</h3>
<pre>
valid: $$ format $$$$ (represents $$)
valid: %% format %%%% (represents %%)
valid: &laquo;$macro&raquo; format $$macro (represents macro value)
valid: &laquo;%macro&raquo; format %%macro (represents macro value)
valid: &laquo;$(macro with spaces in name)&raquo; format $$(macro with spaces in name) (represents macro value)
valid: &laquo;${macro with spaces in name}&raquo; format %%{macro with spaces in name} (represents macro value)
valid: &laquo;$_____&raquo; format $$_____ ($$+only underscores in name)
valid: &laquo;%_____&raquo; format %%_____ (%%+only underscores in name)
</pre>

<h3>Values read from predefs.txt</h3>

<pre>
abc: ${abc}
abc2: ${abc2}
вложенные макросы abc3: ${abc3} 
hello: ${hello}
строка: ${строка} с кириллицей в именах макросов пока не очень дружим :( Чичас поправим
</pre>

