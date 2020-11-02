TEMPLATE = subdirs

SUBDIRS += \
    App \
    Functional \
    LP_Plugin_Import \
    Model

App.depends = Functional
Functional.depends = Model
