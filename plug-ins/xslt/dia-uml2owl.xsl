<?xml version='1.0'?><!-- -*- mode: indented-text;-*- -->
<xsl:transform
    xmlns:xsl  ="http://www.w3.org/1999/XSL/Transform" version="1.0"
    xmlns:r    ="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:s    ="http://www.w3.org/2000/01/rdf-schema#"
    xmlns:dia  ="http://www.lysator.liu.se/~alla/dia/"
    xmlns:owl  ="http://www.w3.org/2002/07/owl@@#"
    >

<div xmlns="http://www.w3.org/1999/xhtml">
<p>dia2owl -- convert UML diagram from dia to OWL</p>

<p>cf <a href="http://www.swi.psy.uva.nl/usr/Schreiber/docs/owl-uml/owl-uml.html">A UML Presentation Syntax for OWL Lite</a>
Author: Guus Schreiber
Created:: April 3, 2002
Last update: April 19, 2002 
</p>

<address>Dan Connolly <br class=""/>
$Id$</address>

<p>see log at end</p>

<p>Share and Enjoy.
  Copyright (c) 2001 W3C (MIT, INRIA, Keio)
   <a href="http://www.w3.org/Consortium/Legal/copyright-software-19980720">Open Source license</a>
</p>

</div>

<xsl:output method="xml" indent="yes"/>

<xsl:template match="/">
  <r:RDF>
    <xsl:apply-templates/>
  </r:RDF>
</xsl:template>


<xsl:template match='dia:object[@type="UML - Class"]'>
  <xsl:variable name="id" select='@id'/>

  <!-- hm... dia seems to have odd string quoting -->
  <xsl:variable name="label" select='substring-before(substring-after(dia:attribute[@name="name"]/dia:string, "#"), "#")'/>

  <xsl:message>@@found UML Class: <xsl:value-of select='$label'/></xsl:message>

  <s:Class r:ID='{$id}'>
    <s:label><xsl:value-of select='$label'/></s:label>
  </s:Class>

</xsl:template>

<xsl:template match='dia:object[@type="UML - Generalization"]'>
  <xsl:variable name="subjID"
                select='dia:connections/dia:connection[@handle="1"]/@to'/>
  <xsl:variable name="objID"
                select='dia:connections/dia:connection[@handle="0"]/@to'/>

  <xsl:message>@@found UML Generalization:
	<xsl:value-of select='$subjID'/>
	<xsl:value-of select='$objID'/>
  </xsl:message>

  <r:Description r:about='{concat("#", $subjID)}'>
    <s:subClassOf r:resource='{concat("#", $objID)}'/>
  </r:Description>

</xsl:template>


<xsl:template match='dia:object[@type="UML - Association"]'>
  <xsl:variable name="id" select='@id'/>
  <xsl:variable name="label" select='substring-before(substring-after(dia:attribute[@name="name"]/dia:string, "#"), "#")'/>

  <xsl:variable name="domainID"
                select='dia:connections/dia:connection[@handle="1"]/@to'/>
  <xsl:variable name="rangeID"
                select='dia:connections/dia:connection[@handle="0"]/@to'/>

  <!-- these include the funky #string quotes# -->
  <xsl:variable name="domainMult"
                select='dia:attribute[@name="ends"]/dia:composite[1]/
                         dia:attribute[@name="multiplicity"]/dia:string'/>
  <xsl:variable name="rangeMult"
                select='dia:attribute[@name="ends"]/dia:composite[2]/
                         dia:attribute[@name="multiplicity"]/dia:string'/>


  <xsl:message>@@found UML Association: <xsl:value-of select='$label'/>
  </xsl:message>

  <r:Property r:ID='{$id}'>
    <s:label><xsl:value-of select='$label'/></s:label>
    <s:domain r:resource='{concat("#", $domainID)}'/>
    <s:range  r:resource='{concat("#", $rangeID)}'/>
  </r:Property>

  <xsl:if test='$rangeMult = "#1..*#"'>
    <r:Description r:about='{concat("#", $domainID)}'>
      <owl:hasAtLeastOne r:resource='{concat("#", $id)}'/>
    </r:Description>
  </xsl:if>

  <!-- @@ more cardinality stuff? -->

</xsl:template>


<!-- don't pass text thru -->
<xsl:template match="text()|@*">
</xsl:template>


<!--
$Log$
Revision 1.1  2003/12/04 07:38:34  lclausen
Doc install fix, new XSLT

Revision 1.2  2002/07/11 08:01:46  connolly
got the objProp case working

Revision 1.1  2002/07/11 07:25:28  connolly
got 1st test case working

-->

</xsl:transform>
