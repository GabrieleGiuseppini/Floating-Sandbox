@echo off
xgettext --c++ --from-code=cp1251 --keyword=_ --sort-output --no-wrap --omit-header --escape -o..\Localization\master\ui_strings.pot ..\FloatingSandbox\*.cpp
