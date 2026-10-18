#include "UIElement.h"
#include "Form.h"

void UIElement::restoreTopLayer()
{
    if (ownerForm && !isOnTop())
        ownerForm->redrawTopLayer();
}

bool UIElement::focus()
{
    return ownerForm && ownerForm->setFocusedElement(this);
}
