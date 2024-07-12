#include "Utils.h"


QString
stripComments(const QString& line)
{
    int cpos;

    cpos = line.indexOf('#');
    if (cpos < 0) {
        cpos = line.indexOf(';');
    }

    if (cpos >= 0) {
        return line.left(cpos).trimmed();
    }

    return line.trimmed();
}
