<?xml version="1.0" encoding="US-ASCII"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- <xsl:import href="/usr/share/xml/docbook/stylesheet/nwalsh/xhtml/chunk.xsl"></xsl:import> -->
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/xhtml/chunk.xsl"></xsl:import>

<xsl:param name="make.valid.html" select="1"></xsl:param>
<xsl:param name="html.cleanup" select="1"></xsl:param>
<xsl:param name="generate.id.attributes" select="1"></xsl:param>

<xsl:param name="html.stylesheet" select="'css/dia.css'"></xsl:param>

<xsl:param name="preface.autolabel" select="i"></xsl:param>
<xsl:param name="component.label.includes.part.label" select="0"></xsl:param>
<xsl:param name="section.autolabel" select="1"></xsl:param>
<xsl:param name="label.from.part" select="1"></xsl:param>
<xsl:param name="section.label.includes.component.label" select="1"></xsl:param>

<xsl:param name="admon.graphics" select="1"></xsl:param>
<xsl:param name="admon.textlabel" select="0"></xsl:param>
<xsl:param name="admon.style">
  <xsl:text></xsl:text>
</xsl:param>

<xsl:param name="callout.unicode" select="1"></xsl:param>

<xsl:param name="formal.title.placement">
figure after
example before
equation before
table before
procedure before
task before
</xsl:param>
<xsl:attribute-set name="formal.title.properties" use-attribute-sets="normal.para.spacing">
<xsl:attribute name="hyphenate">false</xsl:attribute>
</xsl:attribute-set>

</xsl:stylesheet>
