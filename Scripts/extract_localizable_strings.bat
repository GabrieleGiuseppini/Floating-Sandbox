@echo off
xgettext --c++ --from-code=cp1251 --keyword=_ --sort-output --no-wrap --omit-header --escape -o..\Localization\ui_strings.pot ..\FloatingSandbox\*.cpp ..\ShipBuilderLib\*.cpp ..\ShipBuilder\*.cpp ..\UILib\*.cpp
