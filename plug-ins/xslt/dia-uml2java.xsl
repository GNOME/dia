<?xml version="1.0"?>
<!-- 
     Transform dia UML objects to Java classes 

     Copyright(c) 2002 Matthieu Sozeau <mattam@netcourrier.com>

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

  <xsl:output method="text"/>

  <xsl:param name="directory"/>

  <xsl:param name="ident">
    <xsl:text>    </xsl:text>
  </xsl:param>

  <!-- Default visibility of attrs and ops -->
  <xsl:param name="visibility" select="'private'"/>
    
  <xsl:template match="package">
    <xsl:text>package </xsl:text>
    <xsl:value-of select="name"/>
    <xsl:text>;&#xa;</xsl:text>    
    <xsl:apply-templates/>
  </xsl:template>

  
  <xsl:template match="class">
    <xsl:document href="{$directory}{@name}.java" method="text">	
      <xsl:if test="comment">
	<xsl:text>/* &#xa;   </xsl:text>
	<xsl:value-of select="comment"/>
	<xsl:text>&#xa; */&#xa;&#xa;</xsl:text>
      </xsl:if>

      <xsl:text>&#xa;</xsl:text>		
      <xsl:if test="@abstract">		
	<xsl:text>abstract </xsl:text>
      </xsl:if>
      <xsl:text>class </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="@stereotype!=''">
	<xsl:text> : implements </xsl:text>
	<xsl:value-of select="@stereotype"/>      
      </xsl:if>
      <xsl:text> {&#xa;&#xa;</xsl:text>
      <xsl:apply-templates select="attributes"/>
      <xsl:text>&#xa;</xsl:text>
      <xsl:apply-templates select="operations"/>
      <xsl:text>&#xa;}&#xa;</xsl:text>
    </xsl:document>
  </xsl:template>
  
  <xsl:template match="operations|attributes">
    <xsl:if test="*[@visibility='private']">
      <xsl:apply-templates select="*[@visibility='private']"/>
    </xsl:if>
    <xsl:if test="*[@visibility='protected']">
      <xsl:apply-templates select="*[@visibility='protected']"/>
    </xsl:if>
    <xsl:if test="*[@visibility='public']">
      <xsl:apply-templates select="*[@visibility='public']"/>
    </xsl:if>
    <xsl:if test="*[not(@visibility)]">
      <xsl:apply-templates select="*[not(@visibility)]"/>
    </xsl:if>
  </xsl:template>


  <xsl:template match="attribute">
    <xsl:if test="comment!=''">
      <xsl:text>&#xa;</xsl:text>
      <xsl:value-of select="$ident"/>
      <xsl:text>/*&#xa;</xsl:text>
      <xsl:value-of select="$ident"/>
      <xsl:text> * </xsl:text>
      <xsl:value-of select="comment"/>
      <xsl:text>&#xa;</xsl:text>
      <xsl:value-of select="$ident"/>
      <xsl:text> */&#xa;&#xa;</xsl:text>
    </xsl:if>

    <xsl:value-of select="$ident"/>
    <xsl:if test="not(@visibility)">
      <xsl:if test="$visibility=''">
        <xsl:message terminate="yes">
          You must set a valid visibility attribute for attribute <xsl:value-of select="name"/>
        </xsl:message>
      </xsl:if>
      <xsl:value-of select="$visibility"/>
    </xsl:if>
    <xsl:value-of select="@visibility"/>
    <xsl:text> </xsl:text>
    <xsl:if test="@class_scope">
      <xsl:text>static </xsl:text>
    </xsl:if>
    <xsl:value-of select="type"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="name"/>
    <xsl:if test="value">
      <xsl:text> = </xsl:text>
      <xsl:value-of select="value"/>
    </xsl:if>
    <xsl:text>;&#xa;</xsl:text>
  </xsl:template>

  <xsl:template match="operation">
    <xsl:if test="comment!=''">
      <xsl:text>&#xa;</xsl:text>
      <xsl:value-of select="$ident"/>
      <xsl:text>/*&#xa;</xsl:text>
      <xsl:value-of select="$ident"/>
      <xsl:text> * </xsl:text>
      <xsl:value-of select="comment"/>
      <xsl:text>&#xa;</xsl:text>
      <xsl:for-each select="parameters/parameter/comment">
	<xsl:value-of select="$ident"/>
	<xsl:text> * @</xsl:text>
	<xsl:value-of select="../name"/>
	<xsl:text>&#xa;</xsl:text>
      </xsl:for-each>
      <xsl:value-of select="$ident"/>
      <xsl:text> */&#xa;&#xa;</xsl:text>
    </xsl:if>

    <xsl:value-of select="$ident"/>
    <xsl:if test="not(@visibility)">
      <xsl:if test="$visibility=''">
        <xsl:message terminate="yes">
          You must set a valid visibility attribute for method <xsl:value-of select="name"/>
        </xsl:message>
      </xsl:if>
      <xsl:value-of select="$visibility"/>
    </xsl:if>

    <xsl:value-of select="@visibility"/>
    <xsl:text> </xsl:text>

    <xsl:choose>
      <xsl:when test="@inheritance='leaf'">
        <xsl:text>final </xsl:text>
      </xsl:when>
      <xsl:when test="@inheritance='abstract'">
        <xsl:text>abstract </xsl:text>
      </xsl:when>      
    </xsl:choose>
    
    <xsl:if test="@class_scope">
      <xsl:text>static </xsl:text>
    </xsl:if>
    
    <!-- Constructor has no type -->
    <xsl:if test="type">
      <xsl:value-of select="type"/>      
      <xsl:text> </xsl:text>
    </xsl:if>

    <xsl:value-of select="name"/>

    <xsl:text>(</xsl:text>
    <xsl:for-each select="parameters/*">
      <xsl:choose>
        <xsl:when test="@kind='in'">
          <xsl:text>const </xsl:text>
        </xsl:when>
      </xsl:choose>

      <xsl:value-of select="type"/>
      
      <xsl:text> </xsl:text>
      <xsl:value-of select="name"/>
      <xsl:if test="not(position()=last())">
        <xsl:text>, </xsl:text>        
      </xsl:if>
    </xsl:for-each>

    <xsl:text>)</xsl:text>
    <xsl:text> {&#xa;</xsl:text>
    <xsl:value-of select="$ident"/>
    <xsl:text>&#xa;</xsl:text>
    <xsl:value-of select="$ident"/>
    <xsl:text>}&#xa;&#xa;</xsl:text>
  </xsl:template>


  <xsl:template match="text()">    
  </xsl:template>

  <xsl:template match="node()|@*">
    <xsl:apply-templates select="node()|@*"/>  
  </xsl:template>

</xsl:stylesheet>
