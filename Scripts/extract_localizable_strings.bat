@echo off
xgettext --c++ --from-code=cp1251 --keyword=_ --sort-output --no-wrap --omit-header --escape -o..\Localization\ui_strings.pot ..\Sources\FloatingSandbox\*.cpp ..\Sources\ShipBuilderLib\*.cpp ..\Sources\ShipBuilderLib\UI\*.cpp ..\Sources\ShipBuilder\*.cpp ..\Sources\UILib\*.cpp
