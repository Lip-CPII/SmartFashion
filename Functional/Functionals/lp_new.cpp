#include "lp_new.h"
#include "lp_document.h"
#include "renderer/lp_glrenderer.h"

#include "Commands/lp_commandmanager.h"

REGISTER_FUNCTIONAL_IMPLEMENT(LP_New, New, menuFile)

LP_New::LP_New(QObject *parent) : LP_Functional(parent)
{

}

bool LP_New::Run()
{
    auto it = g_Renderers.begin();    //Pick any renderer since all contexts are shared
    Q_ASSERT(it == g_Renderers.cbegin());
    auto renderer = it.value();
    Q_ASSERT(renderer);

    QMetaObject::invokeMethod(renderer,
                              "clearScene",
                              Qt::BlockingQueuedConnection);

    /**
     * @brief Clear the document container. Should be called after
     * clearing up the GL resources
     */
    auto pDoc = &LP_Document::gDoc;
    pDoc->ResetDocument();

    emit pDoc->updateTreeView();
    /**
     * @brief Clear the stack at last since the actual objects
     * are created and stored in the COMMANDs that generates it.
     * Remarks: Event COMMANDs removing objects from "Document"
     * should not delete the object instantly because undoing
     * a delete-COMMAND does not know how the objects are created.
     */
    auto stack = LP_CommandGroup::gCommandGroup->ActiveStack();
    stack->clear();

    LP_GLRenderer::UpdateAll();

    return true;
}
