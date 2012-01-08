<?xml version="1.0"?>
<!-- 
	 Transform dia UML objects to Haxe classes 

	 Copyright(c) 2011 Frank Endres / Education Nationale <frank.endres@ac-nantes.fr>

	 This program is free software; you can redistribute it and/or modify
	 it under the terms of the CeCill License version 2 of the License
	 (http://www.cecill.info/licences/Licence_CeCILL_V2-en.txt)
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="text"/>
	<xsl:param name="directory"/>
	<xsl:param name="ident">
		<xsl:text>    </xsl:text>
	</xsl:param>
	
	<xsl:template match="package">
		<xsl:text>package </xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text>;&#xa;</xsl:text>    
		<xsl:apply-templates/>
	</xsl:template>

	<xsl:template match="class">
		<xsl:document href="{$directory}{@name}.hx" method="text">	
			<xsl:if test="comment">
				<xsl:text>/* &#xa;   </xsl:text>
				<xsl:value-of select="comment"/>
				<xsl:text>&#xa; */</xsl:text>
			</xsl:if>

			<xsl:text>&#xa;</xsl:text>
			<xsl:choose>
				<xsl:when test="@abstract">
					<xsl:text>interface </xsl:text>
				</xsl:when>
				<xsl:otherwise>
					<xsl:text>class </xsl:text>
				</xsl:otherwise>
			</xsl:choose>	
			<xsl:value-of select="@name"/>
			<xsl:if test="@stereotype!=''">
				<xsl:text>, implements </xsl:text>
				<xsl:value-of select="@stereotype"/>      
			</xsl:if>
			<xsl:text> {&#xa;</xsl:text>
			<xsl:apply-templates select="attributes"/>
			<xsl:text>&#xa;</xsl:text>
			<xsl:apply-templates select="operations"/>
			<xsl:text>}&#xa;</xsl:text>
		</xsl:document>
	</xsl:template>
	
	<xsl:template match="operations|attributes">
		<xsl:if test="*[@visibility='private']">
			<xsl:apply-templates select="*[@visibility='private']"/>
		</xsl:if>
		<xsl:if test="*[@visibility='public']">
			<xsl:apply-templates select="*[@visibility='public']"/>
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
			<xsl:text> */&#xa;</xsl:text>
		</xsl:if>

		<xsl:value-of select="$ident"/>
		<xsl:if test="@class_scope">
			<xsl:text>static </xsl:text>
		</xsl:if>
		<xsl:if test="@visibility='public'">
			<xsl:text>public </xsl:text>
		</xsl:if>
		
		<xsl:text>var </xsl:text>
		<xsl:value-of select="name"/>
		<xsl:text> : </xsl:text>
		<xsl:value-of select="type"/>
		
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
			<!--<xsl:for-each select="parameters/parameter/comment">
				<xsl:value-of select="$ident"/>
				<xsl:text> * @</xsl:text>
				<xsl:value-of select="../name"/>
				<xsl:text>&#xa;</xsl:text>
			</xsl:for-each>-->
			<xsl:value-of select="$ident"/>
			<xsl:text> */&#xa;</xsl:text>
		</xsl:if>

		<xsl:value-of select="$ident"/>
		<xsl:if test="@class_scope">
			<xsl:text>static </xsl:text>
		</xsl:if>
		<xsl:if test="@visibility='public'">
			<xsl:text>public </xsl:text>
		</xsl:if>
		<xsl:text>function </xsl:text>
		<xsl:value-of select="name"/>

		<xsl:text>(</xsl:text>
		<xsl:for-each select="parameters/*">
			<xsl:if test="value">
				<xsl:text>?</xsl:text>
			</xsl:if>
			<xsl:value-of select="name"/>
			<xsl:text> : </xsl:text>
			<xsl:value-of select="type"/>
			
			<xsl:if test="value">
				<xsl:text> = </xsl:text>
				<xsl:value-of select="value"/>
			</xsl:if>
		
			<xsl:if test="not(position()=last())">
				<xsl:text>, </xsl:text>        
			</xsl:if>
		</xsl:for-each>

		<xsl:text>)</xsl:text>
		<xsl:if test="type">
			<xsl:text> : </xsl:text>
			<xsl:value-of select="type"/>      
		</xsl:if>
		<xsl:choose>
			<xsl:when test="../../@abstract">
				<xsl:text>;&#xa;&#xa;</xsl:text>
			</xsl:when>
			<xsl:otherwise>
				<xsl:text> {&#xa;</xsl:text>
				<xsl:value-of select="$ident"/>
				<xsl:text>&#xa;</xsl:text>
				<xsl:value-of select="$ident"/>
				<xsl:text>}&#xa;&#xa;</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>


	<xsl:template match="text()">    
	</xsl:template>

	<xsl:template match="node()|@*">
	<xsl:apply-templates select="node()|@*"/>  
	</xsl:template>

</xsl:stylesheet>
