<?xml version="1.0"?>
<!-- 
     Transform dia UML objects to C++ class implementation code. 

     Copyright(c) 2002 Matthieu Sozeau <mattam@netcourrier.com>     
     Copyright(c) 2004 Dave Klotzbach <dklotzbach@foxvalley.net>     

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
<!--

This version is radically different from the c++ translation provided by
dia-uml2c++.xsl. So different that I did not want to replace the old one so I
wrote a new one based on the original. This one handles associataions,
generalization, dependancies, parameterized classes and instantiations of
paramameterized classes as well as generating header(.h) and source(.cpp)files.

Class diagrams with spaces in the names are translated into somewhat legal c++
names and more.

-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="text"/>
	<xsl:param name="directory"/>
	<xsl:param name="indentation"><xsl:text>  </xsl:text></xsl:param>

	<xsl:template match="dia-uml">
		<xsl:apply-templates select="class" mode="header"/>
    <xsl:apply-templates select="class" mode="source"/>
	</xsl:template>

	<xsl:template match="class" mode="header">
    <xsl:variable name="CppClass"><xsl:value-of select="translate(@name, ' ','_')"/></xsl:variable>
    <xsl:variable name="classid"><xsl:value-of select="@id"/></xsl:variable>
    <xsl:variable name="DepSet"><xsl:value-of select="../Dependency[@classId=$classid and @stereotype='bind']/@dependsOn"/></xsl:variable>
    <xsl:variable name="dtor"><xsl:text>~</xsl:text><xsl:value-of select="@name"/></xsl:variable>
    <xsl:variable name="ctor"><xsl:value-of select="@name"/></xsl:variable>

		<xsl:document href="{$directory}{$CppClass}.h" method="text">
       <xsl:variable name="Protect">
		         <xsl:value-of select="concat('__',translate(@name, ' abcdefghijklmnopqrstuvwxyz','_ABCDEFGHIJKLMNOPQRSTUVWXYZ'),'_H__')"/>
       </xsl:variable>
       <xsl:call-template name="InterfaceHeader">
          <xsl:with-param name="ClassName"><xsl:value-of select="$CppClass"/></xsl:with-param>
          <xsl:with-param name="GuardCode"><xsl:value-of select="$Protect"/></xsl:with-param>
       </xsl:call-template>
       <xsl:apply-templates select="comment"/>
       <xsl:apply-templates select="../Association[@from!=$classid and @to=$classid ]" mode="includes"/>
       <xsl:apply-templates select="../Generalization[@subclass=$classid]" mode="includes"/>
       <xsl:apply-templates select="../Dependency[@classId=$classid and @stereotype='bind']" mode="includes"/>
       <xsl:text>&#xa;</xsl:text>
       <xsl:text>&#xa;</xsl:text>
       <xsl:choose>
          <xsl:when test="../class[@id=$DepSet and @template='1']">
             <xsl:text>typedef </xsl:text>
             <xsl:value-of select="translate(../class[@id=$DepSet]/@name,' ','_')"/>
             <xsl:text>&lt;</xsl:text><xsl:value-of select="../Dependency[@classId=$classid]/@name"/>
             <xsl:text>&gt;    </xsl:text>
             <xsl:value-of select="$CppClass"/><xsl:text>;&#xa;</xsl:text>
          </xsl:when>
          <xsl:when test="@template='1'">
             <xsl:text>template&lt;</xsl:text>
             <xsl:apply-templates select="TemplateParameters/formalParameter"/>
             <xsl:text>&gt; </xsl:text>
             <xsl:text>class </xsl:text>
			      <xsl:value-of select="$CppClass"/>
             <xsl:text>{&#xa;</xsl:text>
             <xsl:text>public:&#xa;</xsl:text>
             <xsl:choose>
                <xsl:when test="operations/operation/name=$ctor"/>
                <xsl:otherwise>
                   <xsl:value-of select="$indentation"/>
                   <xsl:value-of select="$CppClass"/>
                   <xsl:text>(){};&#xa;</xsl:text>
                </xsl:otherwise>
             </xsl:choose>
             <xsl:choose>
                <xsl:when test="operations/operation/name=$dtor"/>
                <xsl:otherwise>
                   <xsl:text>public:&#xa;</xsl:text>
                   <xsl:value-of select="$indentation"/>
                   <xsl:text>virtual ~</xsl:text>
                   <xsl:value-of select="$CppClass"/>
                   <xsl:text>(){};&#xa;</xsl:text> 
                </xsl:otherwise>
             </xsl:choose>
			      <xsl:apply-templates select="attributes" mode="header"/>
		         <xsl:apply-templates select="operations" mode="template"/>
             <xsl:text>};&#xa;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
             <xsl:text>class </xsl:text>
			      <xsl:value-of select="$CppClass"/>
             <xsl:apply-templates select="../Generalization[@subclass=$classid]" mode="derivations"/>
             <xsl:text>{&#xa;</xsl:text>
             <xsl:text>private:&#xa;</xsl:text>
             <xsl:apply-templates select="../Association[@to=$classid]" mode="attibutes"/>
             <xsl:choose>
                <xsl:when test="operations/operation/name=$ctor"/>
                <xsl:otherwise>
                   <xsl:text>public:&#xa;</xsl:text>
                   <xsl:value-of select="$indentation"/>
                   <xsl:value-of select="$CppClass"/>
                   <xsl:text>();&#xa;</xsl:text>
                </xsl:otherwise>
             </xsl:choose>
             <xsl:choose>
                <xsl:when test="operations/operation/name=$dtor"/>
                <xsl:otherwise>
                   <xsl:text>public:&#xa;</xsl:text>
                   <xsl:value-of select="$indentation"/>
                   <xsl:text>virtual ~</xsl:text>
                   <xsl:value-of select="$CppClass"/>
                   <xsl:text>();&#xa;</xsl:text> 
                </xsl:otherwise>
             </xsl:choose>
			      <xsl:apply-templates select="attributes" mode="header"/>
			      <xsl:apply-templates select="operations" mode="header"/>
             <xsl:text>};&#xa;</xsl:text>
          </xsl:otherwise>
       </xsl:choose>
       <xsl:text>&#xa;</xsl:text>
       <xsl:text>#endif // defined </xsl:text>
			<xsl:value-of select="$Protect"/>
       <xsl:text>&#xa;</xsl:text>
    </xsl:document>
	</xsl:template>

 <xsl:template match="class" mode="source">
    <xsl:variable name="CppClass"><xsl:value-of select="translate(@name, ' ','_')"/></xsl:variable>
    <xsl:variable name="dtor"><xsl:text>~</xsl:text><xsl:value-of select="$CppClass"/></xsl:variable>
    <xsl:variable name="ctor"><xsl:value-of select="$CppClass"/></xsl:variable>
    <xsl:variable name="classid"><xsl:value-of select="@id"/></xsl:variable>
    <xsl:variable name="DepSet"><xsl:value-of select="../Dependency[@classId=$classid and @stereotype='bind']/@dependsOn"/></xsl:variable>

    <xsl:choose>
       <xsl:when test="@template='1'">
       </xsl:when>
       <xsl:when test="../class[@id=$DepSet and @template='1']">
       </xsl:when>
       <xsl:otherwise>
		      <xsl:document href="{$directory}{$CppClass}.cpp" method="text">
             <xsl:call-template name="ImplementationHeader">
                <xsl:with-param name="ClassName"><xsl:value-of select="@name"/></xsl:with-param>
             </xsl:call-template>
             <xsl:text>// &#xa;</xsl:text>
             <xsl:apply-templates select="comment"/>
				   <xsl:text>//&#xa;</xsl:text>
			      <xsl:text>#include "</xsl:text>
			      <xsl:value-of select="$CppClass"/>
			      <xsl:text>.h"&#xa;</xsl:text>
             <xsl:apply-templates select="../Dependency[@classId=$classid]" mode="includes"/>
             <xsl:text>&#xa;&#xa;</xsl:text>
			      <xsl:apply-templates select="attributes" mode="source"/>
             <xsl:choose>
                <xsl:when test="operations/operation/name=$ctor"/>
                <xsl:otherwise>
                <xsl:value-of select="$CppClass"/><xsl:text>::</xsl:text>
                <xsl:value-of select="$CppClass"/>
                <xsl:text>()&#xa;{&#xa;}&#xa;</xsl:text>
                </xsl:otherwise>
             </xsl:choose>
             <xsl:choose>
                <xsl:when test="operations/operation/name=$dtor"/>
                <xsl:otherwise>
                <xsl:value-of select="$CppClass"/><xsl:text>::~</xsl:text>
                <xsl:value-of select="$CppClass"/>
                <xsl:text>()&#xa;{&#xa;}&#xa;</xsl:text>
                </xsl:otherwise>
             </xsl:choose>
			      <xsl:apply-templates select="operations" mode="source"/>
		      </xsl:document>
       </xsl:otherwise>
    </xsl:choose>
 </xsl:template>

	<xsl:template match="attributes|operations" mode="source">
		<xsl:apply-templates select="attribute|operation" mode="source"/>
	</xsl:template>

	<xsl:template match="attribute" mode="source">
		<xsl:if test="@class_scope">
			<xsl:text>static </xsl:text>
			<xsl:value-of select="type"/>
			<xsl:text>	</xsl:text>
			<xsl:value-of select="name"/>
			<xsl:text> = </xsl:text>
			<xsl:choose>
				<xsl:when test="value">
          <xsl:value-of select="value"/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:text>0</xsl:text>
				</xsl:otherwise>
			</xsl:choose>
       <xsl:text>;</xsl:text>
       <xsl:text>  </xsl:text>
       <xsl:apply-templates select="comment"/>
		   <xsl:text>&#xa;</xsl:text>
		</xsl:if>
	</xsl:template> 
	
	<xsl:template match="operation" mode="source">
		<xsl:text>&#xa;</xsl:text>
		<xsl:text>//  Operation: </xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>&#xa;//&#xa;</xsl:text>
		<xsl:text>//  Description:&#xa;</xsl:text>
		<xsl:text>//   </xsl:text>
    <xsl:apply-templates select="comment"/>
		<xsl:text>&#xa;</xsl:text>
		<xsl:text>//  Parameters:&#xa;</xsl:text>
    <xsl:apply-templates select="parameters/parameter" mode="commentDoc"/>
		<xsl:text>//&#xa;&#xa;</xsl:text>
    <xsl:if test="type">
       <xsl:value-of select="type"/>
       <xsl:text> </xsl:text>
    </xsl:if>
		<xsl:value-of select="translate(../../@name,' ','_')"/>
		<xsl:text>::</xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>(</xsl:text>
    <xsl:apply-templates select="parameters/parameter" mode="code"/>
		<xsl:text>)</xsl:text>
		<xsl:text>&#xa;{&#xa;</xsl:text>
		<xsl:if test="type and type!='void'">
			<xsl:value-of select="$indentation"/>
			<xsl:value-of select="type"/>
			<xsl:text>  returnValue = (</xsl:text>
			<xsl:value-of select="type"/>
			<xsl:text>)0;&#xa;</xsl:text>
			<xsl:value-of select="$indentation"/>
			<xsl:text>return returnValue;&#xa;</xsl:text>
		</xsl:if>
		<xsl:text>}&#xa;</xsl:text>
	</xsl:template>


	<xsl:template match="operation" mode="template">
		<xsl:text>&#xa;</xsl:text>
		<xsl:text>//  Operation: </xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>&#xa;//&#xa;</xsl:text>
		<xsl:text>//  Description:&#xa;</xsl:text>
    <xsl:apply-templates select="comment"/>
		<xsl:text>//  Parameters:&#xa;</xsl:text>
    <xsl:apply-templates select="parameters/parameter" mode="commentDoc"/>
		<xsl:text>//&#xa;&#xa;</xsl:text>
		<xsl:value-of select="$indentation"/>
    <xsl:if test="type">
       <xsl:value-of select="type"/>
       <xsl:text> </xsl:text>
    </xsl:if>
		<xsl:value-of select="name"/>
		<xsl:text>(</xsl:text>
    <xsl:apply-templates select="parameters/parameter" mode="code"/>
		<xsl:text>)</xsl:text>
		<xsl:text>{ </xsl:text>
		<xsl:if test="type and type!='void'">
			<xsl:value-of select="type"/>
			<xsl:text>  returnValue = (</xsl:text>
			<xsl:value-of select="type"/>
			<xsl:text>)0;</xsl:text>
			<xsl:text>return returnValue;</xsl:text>
		</xsl:if>
		<xsl:text>};&#xa;</xsl:text>
	</xsl:template>
	
	
	 <xsl:template match="operations|attributes" mode="header">
    <xsl:if test="*[@visibility='private']">
       <xsl:text>private:&#xa;</xsl:text>
       <xsl:apply-templates select="*[@visibility='private']"/>
    </xsl:if>
    <xsl:if test="*[@visibility='protected']">
       <xsl:text>protected:&#xa;</xsl:text>
       <xsl:apply-templates select="*[@visibility='protected']"/>
    </xsl:if>
    <xsl:if test="*[@visibility='public']">
       <xsl:text>public:&#xa;</xsl:text>
       <xsl:apply-templates select="*[@visibility='public']"/>
       <xsl:text>&#xa;</xsl:text>
    </xsl:if>
    <xsl:if test="*[not(@visibility)]">
       <xsl:apply-templates select="*[not(@visibility)]"/>
    </xsl:if>
 </xsl:template> 


 <xsl:template match="attribute">
    <xsl:value-of select="$indentation"/>
    <xsl:if test="@class_scope">
       <xsl:text>static </xsl:text>
    </xsl:if>
    <xsl:value-of select="type"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="name"/>
    <xsl:text>;</xsl:text>
    <xsl:apply-templates select="comment"/>
    <xsl:text>&#xa;</xsl:text>
 </xsl:template>
 
 
 <xsl:template match="operation">
    <xsl:apply-templates select="comment"/>
    <xsl:value-of select="$indentation"/>
    <xsl:choose>
       <xsl:when test="@inheritance='polymorphic'">
          <xsl:text>virtual </xsl:text>
       </xsl:when>
       <xsl:when test="@inheritance='abstract'">
          <xsl:text>virtual </xsl:text>
       </xsl:when>      
    </xsl:choose>

    <xsl:if test="@class_scope">
       <xsl:text>static </xsl:text>
    </xsl:if>
    <xsl:if test="type">
       <xsl:value-of select="type"/>
    </xsl:if>
    <xsl:text> </xsl:text>
    <xsl:value-of select="name"/>
    <xsl:text>(</xsl:text>
    <xsl:apply-templates select="parameters/parameter" mode="code"/>
    <xsl:text>)</xsl:text>
    <xsl:if test="@inheritance='abstract'">
       <xsl:text> = 0</xsl:text>
    </xsl:if>
    <xsl:if test="@query=1">
       <xsl:text> const</xsl:text>
    </xsl:if>
    <xsl:text>;&#xa;</xsl:text>
 </xsl:template>

 <xsl:template match="formalParameter">
    <xsl:value-of select="@class"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:if test="position() != last()">
       <xsl:text>, </xsl:text>
    </xsl:if>
 </xsl:template>

 <xsl:template match="Association" mode="includes">
    <xsl:variable name="from"><xsl:value-of select="@from"/></xsl:variable>
    <xsl:text>#include "</xsl:text>
    <xsl:value-of select="translate(../class[@id=$from]/@name, ' ','_')"/>
    <xsl:text>.h"&#xa;</xsl:text>
 </xsl:template>

 <xsl:template match="Dependency" mode="includes">
    <xsl:variable name="dependsOn"><xsl:value-of select="@dependsOn"/></xsl:variable>
    <xsl:text>#include "</xsl:text>
    <xsl:value-of select="translate(../class[@id=$dependsOn]/@name, ' ','_')"/>
    <xsl:text>.h"&#xa;</xsl:text>
 </xsl:template>

 <xsl:template match="Generalization" mode="includes">
    <xsl:variable name="superclass"><xsl:value-of select="@superclass"/></xsl:variable>
    <xsl:text>#include "</xsl:text>
    <xsl:value-of select="translate(../class[@id=$superclass]/@name, ' ','_')"/>
    <xsl:text>.h"&#xa;</xsl:text>
 </xsl:template>

 <xsl:template match="Generalization" mode="derivations">
    <xsl:text>: public </xsl:text>
    <xsl:variable name="superclass"><xsl:value-of select="@superclass"/></xsl:variable>
    <xsl:value-of select="translate(../class[@id=$superclass]/@name,' ','_')"/>
 </xsl:template>

 <xsl:template match="Association" mode="attibutes">
    <xsl:if test="Aggregate[@role='2']/@aggregate!='none' and Aggregate[@role='1']/@name!=''">
       <xsl:variable name="from"><xsl:value-of select="@from"/></xsl:variable>
       <xsl:value-of select="$indentation"/>
       <xsl:value-of select="translate(../class[@id=$from]/@name,' ','_')"/>
       <xsl:text>  </xsl:text>
       <xsl:if test="Aggregate[@role='2']/@aggregate='aggregation'">
          <xsl:text>* </xsl:text>
       </xsl:if>
       <xsl:value-of select="translate(Aggregate[@role='1']/@name,' ','_')"/>
       <xsl:text>;&#xa;</xsl:text>
    </xsl:if>
 </xsl:template>

 <xsl:template match="parameter" mode="commentDoc">
	   <xsl:text>//      </xsl:text>
	   <xsl:value-of select="name"/>
	   <xsl:text>	</xsl:text>
	   <xsl:value-of select="comment"/>
	   <xsl:text>&#xa;</xsl:text>
 </xsl:template>

 <xsl:template match="parameter" mode="code">
	   <xsl:choose>
		   <xsl:when test="@kind='in'">
			   <xsl:text>const </xsl:text>
		   </xsl:when>
	   </xsl:choose>
	   <xsl:value-of select="type"/>
	   <xsl:if test="@kind='out'">
		   <xsl:text>	&amp;</xsl:text>
	   </xsl:if>
    <xsl:text> </xsl:text>
	   <xsl:value-of select="name"/>
	   <xsl:if test="not(position()=last())">
		   <xsl:text>, </xsl:text>
	   </xsl:if>
 </xsl:template>


 <xsl:template match="comment">
    <xsl:if test=".!=''">
       <xsl:text>// </xsl:text>
       <xsl:value-of select="."/>
       <xsl:text>&#xa;</xsl:text>
    </xsl:if>
 </xsl:template>

	<xsl:template match="text()">
	</xsl:template>

	<xsl:template match="node()|@*">
		<xsl:apply-templates match="node()|@*"/>
	</xsl:template>

 <xsl:template name="InterfaceHeader"><xsl:param name="ClassName"/><xsl:param name="GuardCode"/>
		<xsl:text>// </xsl:text>
		<xsl:value-of select="$ClassName"/>
		<xsl:text>.h
//
// This header file defines the interfaces to the class </xsl:text><xsl:value-of select="@name"/><xsl:text>
//
// This file was generate from a Dia Diagram using the xslt script file dia-uml2cpp.xsl
// which is copyright(c) Dave Klotzbach &lt;dklotzbach@foxvalley.net&gt;
//
// The author asserts no additional copyrights on the derived product. Limitations
// on the uses of this file are the right and responsibility of authors of the source 
// diagram.
// 
// The dia-uml2cpp.xsl script is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
// more details.
//
// A copy of the GNU General Public License is available by writing to the 
// Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
// MA 02111-1307, USA.
//

</xsl:text>
    <xsl:text>#ifndef </xsl:text>
		<xsl:value-of select="$GuardCode"/>
		<xsl:text>&#xa;</xsl:text>
    <xsl:text>#define </xsl:text>
		<xsl:value-of select="$GuardCode"/>
    <xsl:text>&#xa;&#xa;&#xa;</xsl:text>
 </xsl:template>	
	
 <xsl:template name="ImplementationHeader"><xsl:param name="ClassName"/>
		<xsl:text>// </xsl:text>
		<xsl:value-of select="$ClassName"/>
		<xsl:text>.cpp
//
// This header file defines the implementation of the class </xsl:text><xsl:value-of select="@name"/><xsl:text>
//
// This file was generate from a Dia Diagram using the xslt script file dia-uml2cpp.xsl
// which is copyright(c) Dave Klotzbach &lt;dklotzbach@foxvalley.net&gt;
//
// The author asserts no additional copyrights on the derived product. Limitations
// on the uses of this file are the right and responsibility of authors of the source 
// diagram.
// 
// The dia-uml2cpp.xsl script is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
// more details.
//
// A copy of the GNU General Public License is available by writing to the 
// Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
// MA 02111-1307, USA.
//
</xsl:text>
 </xsl:template>	
</xsl:stylesheet>
