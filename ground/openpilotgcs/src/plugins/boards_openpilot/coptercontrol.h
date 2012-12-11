/**
 ******************************************************************************
 *
 * @file       coptercontrol.h
 * @author     The PhoenixPilot Team, http://github.com/PhoenixPilot Copyright (C) 2012.
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup Boards_OpenPilotPlugin OpenPilot boards support Plugin
 * @{
 * @brief Plugin to support boards by the OP project
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef COPTERCONTROL_H
#define COPTERCONTROL_H

#include <coreplugin/iboardtype.h>

class IBoardType;

class CopterControl : public Core::IBoardType
{
public:
    CopterControl();
    virtual ~CopterControl();

    virtual QString shortName();
    virtual QString boardDescription();
    virtual QStringList getSupportedProtocols();

};


#endif // COPTERCONTROL_H
