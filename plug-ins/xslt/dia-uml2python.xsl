<?xml version="1.0"?>
<!-- 
     Transform dia UML objects to Python classes 

     Based on The uml2c++ code from
     Matthieu Sozeau <mattam@netcourrier.com>
     Copyright(c) 2003 Holger Lehmann <holger.lehmann@catworkx.de>

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

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:output method="text"/>

  <xsl:param name="directory"/>
    
  <xsl:param name="ident">
    <xsl:text>    </xsl:text>
  </xsl:param>
  
  <!-- Visibility indentation -->
  <xsl:param name="visident">
    <xsl:text>  </xsl:text>
  </xsl:param>

  <xsl:template match="class">
    <xsl:document href="{$directory}{@name}.py" method="text">
      <xsl:if test="comment">
	<xsl:text># </xsl:text>
	<xsl:value-of select="comment"/>
	<xsl:text>&#xa;</xsl:text>
      </xsl:if>

      <xsl:text>class </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text> ( </xsl:text>
      <xsl:if test="@stereotype!=''">
        <xsl:value-of select="@stereotype"/>      
      </xsl:if>
      <xsl:text> ):&#xa;</xsl:text>
      <xsl:if test="comment!=''">
        <xsl:value-of select="$ident"/>
        <xsl:text>"""&#xa;</xsl:text>
        <xsl:value-of select="$ident"/>
        <xsl:text>  </xsl:text>
        <xsl:value-of select="comment"/>
        <xsl:text>&#xa;</xsl:text>
        <xsl:value-of select="$ident"/>
        <xsl:text>"""&#xa;</xsl:text>
      </xsl:if>
      <xsl:apply-templates select="attributes"/>
      <xsl:text>&#xa;</xsl:text>
      <xsl:apply-templates select="operations"/>
    </xsl:document>
  </xsl:template>
  
  <xsl:template match="operations|attributes">
    <xsl:if test="*[@visibility='private']">
      <xsl:value-of select="$visident"/>
      <xsl:text>  #private:&#xa;</xsl:text>
      <xsl:apply-templates select="*[@visibility='private']"/>
      <xsl:text>&#xa;</xsl:text>
    </xsl:if>
    <xsl:if test="*[@visibility='protected']">
      <xsl:value-of select="$visident"/>
      <xsl:text>  #protected:&#xa;</xsl:text>
      <xsl:apply-templates select="*[@visibility='protected']"/>
      <xsl:text>&#xa;</xsl:text>
    </xsl:if>
    <xsl:if test="*[@visibility='public']">
      <xsl:value-of select="$visident"/>
      <xsl:text>  #public:&#xa;</xsl:text>
      <xsl:apply-templates select="*[@visibility='public']"/>
      <xsl:text>&#xa;</xsl:text>
    </xsl:if>
    <xsl:if test="*[not(@visibility)]">
      <xsl:apply-templates select="*[not(@visibility)]"/>
      <xsl:text>&#xa;</xsl:text>
    </xsl:if>
  </xsl:template>


  <xsl:template match="attribute">

    <xsl:value-of select="$ident"/>
    <xsl:value-of select="name"/>
    <xsl:if test="value!=''">
      <xsl:text> = </xsl:text>
      <xsl:value-of select="value"/>
    </xsl:if>
    <xsl:if test="value=''">
      <xsl:text> = None </xsl:text>
    </xsl:if>
    <xsl:text> # </xsl:text>
    <xsl:value-of select="type"/>
    <xsl:if test="comment!=''">
      <xsl:text>&#xa;</xsl:text>
      <xsl:value-of select="$ident"/>
      <xsl:text>"""&#xa;</xsl:text>
      <xsl:value-of select="$ident"/>
      <xsl:text>  </xsl:text>
      <xsl:value-of select="comment"/>
      <xsl:text>&#xa;</xsl:text>
      <xsl:value-of select="$ident"/>
      <xsl:text>"""</xsl:text>
    </xsl:if>
    <xsl:text>&#xa;</xsl:text>
  </xsl:template>

  <xsl:template match="operation">

    <xsl:value-of select="$ident"/>
    
    <xsl:value-of select="name"/>
    <xsl:text>(</xsl:text>
    <xsl:for-each select="parameters/*">
      <xsl:text> </xsl:text>
      <xsl:value-of select="name"/>
      <xsl:if test="value">
        <xsl:text> = </xsl:text>
        <xsl:value-of select="value"/>
      </xsl:if>
      <xsl:if test="not(position()=last())">
        <xsl:text>, </xsl:text>        
      </xsl:if>
    </xsl:for-each>

    <xsl:text>):</xsl:text>
    <!-- Constructor has no type -->
    <xsl:if test="type">
      <xsl:text> # returns:  </xsl:text>      
      <xsl:value-of select="type"/>
    </xsl:if>
    <xsl:text> &#xa; </xsl:text>      

    <xsl:value-of select="$ident"/>
    <xsl:text>   """&#xa;</xsl:text>
    <xsl:if test="comment!=''">
      <xsl:value-of select="$ident"/>
      <xsl:text>     </xsl:text>
      <xsl:value-of select="comment"/>
      <xsl:text>&#xa;&#xa;</xsl:text>
    </xsl:if>
    <xsl:for-each select="parameters/parameter/comment">
      <xsl:value-of select="$ident"/>
      <xsl:text>     @</xsl:text>
      <xsl:value-of select="../name"/>
      <xsl:text>:</xsl:text>
      <xsl:value-of select="../type"/>
      <xsl:if test="../comment!=''">
	<xsl:text> :</xsl:text>
	<xsl:value-of select="../comment"/>
      </xsl:if>
      <xsl:text>&#xa;</xsl:text>
    </xsl:for-each>
    <xsl:if test="type">
      <xsl:text>         returns:  </xsl:text>      
      <xsl:value-of select="type"/>
      <xsl:text>&#xa;</xsl:text>
    </xsl:if>
    <xsl:value-of select="$ident"/>
    <xsl:text>    """&#xa;</xsl:text>
    <xsl:text>       return None&#xa;&#xa;</xsl:text>

  </xsl:template>


  <xsl:template match="text()">    
  </xsl:template>

  <xsl:template match="node()|@*">
    <xsl:apply-templates select="node()|@*"/>  
  </xsl:template>

</xsl:stylesheet>
<!-- Keep this comment at the end of the file
Local variables:
mode: xml
sgml-omittag:nil
sgml-shorttag:nil
sgml-namecase-general:nil
sgml-general-insert-case:lower
sgml-minimize-attributes:nil
sgml-always-quote-attributes:t
sgml-indent-step:2
sgml-indent-data:t
sgml-parent-document:nil
sgml-exposed-tags:nil
sgml-local-catalogs:nil
sgml-local-ecat-files:nil
End:
-->
