<?xml version="1.0"?>
<!-- 
     Generate a grouped list of dia UML components 

     Copyright(c) 2003 Steffen Macke <sdteffen@web.de>

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <xsl:output method="xml"/> 
  <xsl:key name="component-by-text" match="component" use="."/>
  <xsl:key name="component-by-stereotype" match="component" use="@stereotype" />
  <xsl:template match="/dia-uml">
  <html>
    <head><title>Component List</title></head>
    <body>
      <h1>Component List</h1>
      <table border="1">
        <tr>
	  <th>Stereotype</th>
	  <th>Component</th>
	  <th>Count</th>
	</tr>
	<xsl:for-each select=
	  "component[count(.|key('component-by-stereotype',@stereotype)[1])=1]">
	  <xsl:sort select="@stereotype" />
	  <xsl:variable name="stereotype"><xsl:value-of select="@stereotype" 
	    /></xsl:variable>
	  <xsl:for-each select=
	  "../component[@stereotype=$stereotype and (count(. | key('component-by-text', .)[1]) = 1)]">
	    <xsl:sort select="text()" />
	    <tr>
	      <td><xsl:value-of select="$stereotype" /></td>
	      <td><xsl:value-of select="." /></td>
	      <td><xsl:value-of select="count(key('component-by-text', .))" />
	      </td>
	    </tr>
	  </xsl:for-each>
	</xsl:for-each>
      </table>
    </body>
  </html>
  </xsl:template>
</xsl:stylesheet>
