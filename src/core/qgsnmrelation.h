/***************************************************************************

               ----------------------------------------------------
              date                 : 16.9.2015
              copyright            : (C) 2015 by Matthias Kuhn
              email                : matthias (at) opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrelation.h"

/**
 * @brief The QgsNmRelation class represents a many-to-many relation based on a fully normalized
 * relational database model.
 */
class CORE_EXPORT QgsNmRelation
{
  public:

    static QgsNmRelation createFromXML( const QDomNode& node );

    QgsNmRelation( const QgsRelation& relationOne, const QgsRelation& relationTwo );
    QgsRelation relationOne() const;
    QgsRelation relationTwo() const;

  private:
    QgsRelation mRelationOne;
    QgsRelation mRelationTwo;
};
