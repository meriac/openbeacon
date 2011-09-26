/***************************************************************
 *
 * OpenBeacon.org - JavaScript Proximity Graph Example Code
 *                  for openbeacon-tracker JSON server interface
 *
 * uses a physical model and statistical analysis to calculate
 * positions of tags and the JavaScript D3 library for
 * visualization - see http://mbostock.github.com/d3/api/
 *
 * Copyright 2011 Milosch Meriac <milosch@openbeacon.de>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published
 by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

var g_AvatarIconIDs = new Array(),
    g_Matrix = new Array(),
    g_MaxID = 1000000;

function chord_draw(Matrix,Keys,Comment) {
	var chord = d3.layout.chord()
	    .padding(.05)
	    .matrix(Matrix)

	var w = 900, h = 900,
	    r1 = Math.round(Math.min(w, h) * .38),
	    r0 = Math.round(r1 / 1.1),
	    fill = d3.scale.category20();

	function my_fill(index)
	{
	    return fill( Keys[index] % 20 );
	}

	var svg = d3.select("#chart")
	  .text("")
	  .append("svg:svg")
	    .attr("id", "openbeacon")
	    .attr("width", w)
	    .attr("height", h)
	  .append("svg:g")
	    .attr("transform", "translate(" + Math.round(w / 2) + "," + Math.round(h / 2) + ")");

	svg.append("svg:g")
	  .selectAll("path")
	    .data(chord.groups)
	  .enter().append("svg:path")
	    .style("fill", function(d) {
		return my_fill(d.index);
	    })
	    .style("stroke", "#FFF")
	    .attr("d", d3.svg.arc().innerRadius(r0).outerRadius(r1))
	    .on("mouseover", fade(.1))
	    .on("mouseout", fade(1));

	var ticks = svg.append("svg:g")
	  .selectAll("g")
	    .data(chord.groups)
	  .enter().append("svg:g")
	  .selectAll("g")
	    .data(groupTicks)
	  .enter().append("svg:g")
	    .attr("transform", function(d) {
	      return "rotate(" + (d.angle * 180 / Math.PI - 90) + ")"
		  + "translate(" + r1 + ",0)";
	    });

	ticks.append("svg:image")
	    .attr("x", 50)
	    .attr("y", -16)
	    .attr("width", 32)
	    .attr("height", 32)
	    .attr("transform", function(d) {
	      return d.angle > Math.PI ? "rotate(180)translate(-132)" : null;
	    })
	    .attr("xlink:href", function(d) {
		return "http://api.openbeacon.net/images/avatars/"+(g_AvatarIconIDs[d.tag_id]?d.tag_id:'none')+".png";
	    })

	ticks.append("svg:line")
	    .attr("x1", 1)
	    .attr("y1", 0)
	    .attr("x2", 5)
	    .attr("y2", 0)
	    .style("stroke", "#FFF");

	svg.append("svg:text")
	    .attr("fill", "white")
	    .attr("x", -r1)
	    .attr("y", Math.round(h/2))
	    .attr("dy", "-1em")
	    .text(Comment);

	ticks.append("svg:text")
	    .attr("fill", "white")
	    .attr("x", 8)
	    .attr("dy", ".35em")
	    .attr("text-anchor", function(d) {
	      return d.angle > Math.PI ? "end" : null;
	    })
	    .attr("transform", function(d) {
	      return d.angle > Math.PI ? "rotate(180)translate(-16)" : null;
	    })
	    .text(function(d) { return d.label; });

	svg.append("svg:g")
	    .attr("class", "chord")
	  .selectAll("path")
	    .data(chord.chords)
	  .enter().append("svg:path")
	    .style("fill", function(d) {
		return my_fill(d.target.index);
	    })
	    .style("stroke", "#FFF")
	    .attr("d", d3.svg.chord().radius(r0))
	    .style("opacity", 1);

	function groupTicks(d) {
	    return d3.range(0, d.value, 10).map(function(v, i) {
	    tag_id = Keys[d.index];
	    return {
	      angle: (d.startAngle + d.endAngle)/2,
	      tag_id: tag_id,
	      label: "Tag " + tag_id
	    };
	  });
	}

	function fade(opacity) {
	  return function(g, i) {
	    svg.selectAll("g.chord path")
		.filter(function(d) {
		  return d.source.index != i && d.target.index != i;
		})
	      .transition()
		.style("opacity", opacity);
	  };
	}

}

function process_server(json) {

	// update time
	if(json.time)
        {
	    date = new Date();
	    date.setTime(parseInt(json.time)*1000);
	    Comment = date.toGMTString();
        }
        else
	    Comment = undefined;

	// maintain list of unique IDs seen
	for (i in json.edge) {
	    edge = json.edge[i];

	    tag1 = edge.tag[0];
	    tag2 = edge.tag[1];

	    // sort IDs
	    if(tag1>tag2)
	    {
		tmp = tag2;
		tag2 = tag1;
		tag1 = tmp;
	    }

	    // aggregate power levels
	    if(tag2<g_MaxID)
	    {
		index = (tag1*g_MaxID)+tag2;
		if(g_Matrix[index])
		    g_Matrix[index].power+=edge.power;
		else
		    g_Matrix[index]={
			tag1 : tag1,
			tag2 : tag2,
			power: edge.power
		    };
	    }
	}

	// decay of sightings
	var unique = new Array();
	for (i in g_Matrix)
	{
	    edge = g_Matrix[i];

	    power = edge.power * 0.9;
	    if(power<1)
		delete g_Matrix[i];
	    else
	    {
		edge.power = power;
		unique[edge.tag1] = edge.tag1;
		unique[edge.tag2] = edge.tag2;
	    }
	}
	unique.sort();

	// build index lookup list
	count = 0;
	IndexLookup = new Array();
	Keys = new Array();
	for (i in unique)
	{
	    tag_id = unique[i];
	    IndexLookup[tag_id] = count;
	    Keys[count] = tag_id;
	    count++;
	}

	// build empty matrix
	Matrix = new Array();
	for (i=0; i<count; i++)
	{
	    row = new Array(count);
	    for(j=0; j<count; j++)
		row[j] = 0;
	    Matrix[i] = row;
	}

	for (i in g_Matrix)
	{
	    edge = g_Matrix[i];
	    tag_idx1 = IndexLookup[edge.tag1];
	    tag_idx2 = IndexLookup[edge.tag2];
	    Matrix[tag_idx1][tag_idx2]+=edge.power;
	    Matrix[tag_idx2][tag_idx1]+=edge.power;
	}

	chord_draw(Matrix,Keys,Comment);
}

function refresh_svg()
{
  d3.json('http://api.openbeacon.net/get/brucon.json', process_server);
  window.setTimeout(refresh_svg, 500);
}

function get_avatars(avatars)
{
  if(avatars.length)
  {
    g_AvatarIconIDs = new Array();
    for (i in avatars)
	g_AvatarIconIDs[avatars[i]]=true;
  }
  else
    alert("Can't load avatar icon list");

  /* start polling */
  refresh_svg()
}

d3.json('http://api.openbeacon.net/images/avatars/list.json', get_avatars);
