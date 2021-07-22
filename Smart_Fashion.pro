TEMPLATE = subdirs

SUBDIRS += \
    App \
    Functional \
    LP_Plugin_Garment_Manipulation \
    LP_Plugin_Import \
    LP_Plugin_PIFuHD \
    LP_Plugin_Simulator \
    Model

App.depends = Functional
Functional.depends = Model
LP_Plugin_Import.depends = Functional
LP_Plugin_PIFuHD.depends = Functional
LP_Plugin_Simulator.depends = Functional
LP_Plugin_Garment_Manipulation.depends = Functional
