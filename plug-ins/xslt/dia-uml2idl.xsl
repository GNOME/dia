<?xml version="1.0"?>
<!-- 
     Transform dia UML objects to IDL interfaces
     
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

  <!-- Interface indentation -->
  <xsl:param name="interident">
    <xsl:text>  </xsl:text>
  </xsl:param>

  <xsl:param name="warn">true</xsl:param>

  <!-- Default type -->
  <xsl:param name="type" select="'void'"/>
  
  <!-- Default kind of argument passing -->
  <xsl:param name="kind" select="'inout'"/>

  <xsl:template match="package">
    <xsl:text>module </xsl:text>
    <xsl:value-of select="name"/>
    <xsl:text> {&#xa;</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>&#xa;};&#xa;</xsl:text>
  </xsl:template>

  
  <xsl:template match="class">
    <xsl:document href="{$directory}{@name}.idl" method="text">
      <xsl:if test="comment">
	<xsl:text>/* &#xa;   </xsl:text>
	<xsl:value-of select="comment"/>
	<xsl:text>&#xa; */&#xa;&#xa;</xsl:text>
      </xsl:if>
      <xsl:text>&#xa;</xsl:text>	
      <xsl:value-of select="$interident"/>
      <xsl:text>interface </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="not(@stereotype='')">
      <xsl:text> : </xsl:text>
	<xsl:value-of select="@stereotype"/>      
      </xsl:if>
      <xsl:text> {&#xa;&#xa;</xsl:text>
      <xsl:apply-templates select="attributes"/>
      <xsl:text>&#xa;</xsl:text>
      <xsl:apply-templates select="operations"/>
      <xsl:text>&#xa;</xsl:text>
      <xsl:value-of select="$interident"/>
      <xsl:text>};&#xa;</xsl:text>
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

    <xsl:if test="type=''">
      <xsl:if test="$warn='true'">
        <xsl:message>// Defaulting type to <xsl:value-of select="$type"/> for <xsl:value-of select="name"/>().</xsl:message>
      </xsl:if>
      <xsl:value-of select="$type"/>
    </xsl:if>
    
    <xsl:value-of select="type"/>      
    <xsl:text> </xsl:text>  

    <xsl:value-of select="name"/>

    <xsl:text>(</xsl:text>
    <xsl:for-each select="parameters/*">
      <xsl:if test="not(@kind)">
        <xsl:if test="$warn='true'">
          <xsl:message>// Defaulting argument passing kind to <xsl:value-of select="$kind"/> for <xsl:value-of select="name"/>.</xsl:message>    
        </xsl:if>
        <xsl:value-of select="$kind"/>
      </xsl:if>
      <xsl:value-of select="@kind"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="type"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="name"/>
      <xsl:if test="not(position()=last())">
        <xsl:text>, </xsl:text>        
      </xsl:if>
    </xsl:for-each>
    <xsl:text>);&#xa;&#xa;</xsl:text>
  </xsl:template>


  <xsl:template match="text()">    
  </xsl:template>

  <xsl:template match="node()|@*">
    <xsl:apply-templates select="node()|@*"/>  
  </xsl:template>

</xsl:stylesheet>