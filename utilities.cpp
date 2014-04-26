#include "utilities.h"

/*!
 * \brief Extracts the base callsign from a full callsign that includes a prefix or suffix.
 * \param fullCall A QString reprenting a callsign containing a prefix or suffix.  A callsign without a suffix or prefix is
 *                 also acceptable.
 * \return A QString representing the base callsign.
 */
QString baseCall(QString fullCall)
{
    int indexStroke = fullCall.indexOf('/');
    if(indexStroke < 0) {
        return fullCall;
    }

    if(indexStroke >= (fullCall.count()-2)) {
        return fullCall.left(indexStroke);
    }

    return fullCall.mid(indexStroke+1,-1);
}
