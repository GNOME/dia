#!/usr/bin/perl -w -i

use strict;
use XML::LibXML;

my $NS = "http://www.daa.com.au/~james/dia-shape-ns";

sub getAttribute {
    my ($node, $name) = @_;

    for my $attr ($node->attributes()) {
	if ($attr->nodeName eq $name) {
	    return $attr;
	}
    }
}

while (my $filename = shift) {
    my $parser = XML::LibXML->new();
    my $tree = $parser->parse_file($filename);
    my $root = $tree->getDocumentElement; # shape element
    
    my @svg = $root->getElementsByTagName('svg:svg');
    if (@svg > 1) {
	die "Too many svg's";
    }
    if (@svg == 0) {
	# No SVG
	next;
    }
    my $svg = $svg[0]; # should only be one

    my $widthval = $svg->getAttribute("width");
    if ($widthval) {
	$widthval =~ s/cm//;
    }
    my $heightval = $svg->getAttribute("height");
    if ($heightval) {
	$heightval =~ s/cm//;
    }
    if ($widthval && $heightval) {
	my $newline = $tree->createTextNode( "\n" );
	my $midx = $widthval/2;
	my $midy = $heightval/2;
        # For unknown reasons, this doesn't find the connections child

#	my @connections = $root->getElementsByTagName('connections');
#	if (@connections > 1) {
#	    die "Too many connections";
#	}
#	my $conn2 = $connections[0];
#	print "Old conn: " . $conn2 . "\n";

	my @points = $tree->getElementsByTagNameNS($NS, 'point');

	my @conn = $tree->getElementsByTagNameNS($NS, 'connections');
	if (@conn > 1) {
	    die "Too many connection groups";
	}
	my $conn = $conn[0];

	if ($conn) {
	    my @points = $tree->getElementsByTagNameNS($NS, 'point');
	    my $mainPoint = $tree->createElement("point");
	    $mainPoint->setAttribute("x", $midx);
	    $mainPoint->setAttribute("y", $midy);
	    $mainPoint->setAttribute("main", "yes");
	    my $replaced = 0;
	    foreach my $point (@points) {
		my $x = getAttribute($point, "x");
		my $y = getAttribute($point, "y");
		my $main = getAttribute($point, "main");
		if ($main) {
#		    $conn->removeChild($point);
		    $point->replaceNode($mainPoint);
		    $replaced = 1;
		} else {
		    if ($x == $midx && $y == $midy) {
			print "Replacing nonmain\n";
			$conn->removeChild($point);
		    }
		}
	    }
	    if (!$replaced) {
		$conn->addChild($mainPoint);
		$root->insertAfter( $newline, $mainPoint);
	    }
	} else {
	    $conn = $tree->createElement("connections");
	    my $mainPoint = $tree->createElement("point");
	    $mainPoint->setAttribute("x", $midx);
	    $mainPoint->setAttribute("y", $midy);
	    $mainPoint->setAttribute("main", "yes");
	    $conn->addChild($mainPoint);
	    my $spacing = $tree->createTextNode( "  " );
	    $root->insertBefore($spacing, $svg);
	    $root->insertBefore($conn, $svg);
	    $root->insertBefore($newline, $svg);
	}
	open(OUT, ">$filename");
	print OUT $tree->toString;
	close OUT;
    }
}

#my @svg = $shape[0]->getElementsByTagName('svg:svg');
#print @svg;

