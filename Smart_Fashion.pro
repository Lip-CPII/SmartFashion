TEMPLATE = subdirs

SUBDIRS += \
    App \
    Functional \
    LP_Plugin_Import \
    Model

App.depends = Functional
Functional.depends = Model
LP_Plugin_Import.depends = Functional

win32:CONFIG(release, debug|release): {


#QMAKE_EXTRA_TARGETS += customtarget1

#customtarget1.target = dummy
#customtarget1.commands = set PATH=$(PATH)

#PRE_TARGETDEPS += dummy
}
