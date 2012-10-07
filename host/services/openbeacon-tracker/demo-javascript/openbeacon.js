/***************************************************************
 *
 * OpenBeacon.org - JavaScript example code for
 *                  openbeacon-tracker JSON server interface
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

var w = 1024,
    h = 1024,
    fill = d3.scale.category20();
    floor = 1;

var nodes_tag = new Array(),
    nodes_reader = new Array(),
    nodes = new Array(),
    links = new Array();

var replay_date_string = d3.select('#date');
    replay_date = new Date();

var vis = d3.select('#chart')
  .attr('style', 'background-image: url(http://api.openbeacon.net/images/brucon.png); background-repeat: no-repeat;')
  .append('svg:svg')
    .attr('width', w)
    .attr('height', h);

var vislink = null;
var visnode = null;

var force = d3.layout.force()
      .charge(-120)
      .linkDistance(30)
      .nodes(nodes)
      .links(links)
      .gravity(0)
      .size([w, h])
      .on('tick', function() {

	if(vislink)
	  vislink.attr('x1', function(d) { return d.source.x; })
	    .attr('y1', function(d) { return d.source.y; })
	    .attr('x2', function(d) { return d.target.x; })
	    .attr('y2', function(d) { return d.target.y; });

	if(visnode)
	  visnode.attr('cx', function(d) { return d.x; })
	    .attr('cy', function(d) { return d.y; })
	    .style('fill', function(d) { return fill(d.group); });
    });

vis.style('opacity', 1e-6)
  .transition()
    .duration(2000)
    .style('opacity', 1);

function readerid(id)
{
    var num = (id & 0xFF).toString();

    while( num.length < 3 )
	num = '0' + num;

    return String.fromCharCode(65+((id>>8)&0x1F))+num;
}

function process_server(json) {

    var link, node;
    var i, t;
    var tag, reader, oldocount, changed;

    nodesm = new Array(),
    linksm = new Array();

    t = oldcount = nodes.length;
    changed = false;

    if(json.time)
    {
	replay_date.setTime(parseInt(json.time)*1000);
	replay_date_string.text(replay_date.toGMTString());
    }

    for (i in json.reader) {

	reader = json.reader[i];

	if((reader.floor == floor) && !nodes_reader[reader.id])
	{
	    changed = true;

	    node = new Object;
	    node.id = 'R'+reader.id;
	    node.name = 'Reader '+readerid(reader.id);
	    node.group = 2;
	    node.fixed = true;
	    node.index = t;
	    node.radius = 7;
	    node.px = node.x = reader.loc[0];
	    node.py = node.y = reader.loc[1];

	    nodes.push(node);
	    nodesm.push(node);
	    nodes_reader[reader.id] = node;

	    t++;
	}
    }

    for (i in json.tag) {

	tag = json.tag[i];

	if(tag.reader && nodes_reader[tag.reader])
	{
	    node = nodes_tag[tag.id];

	    if(node)
	    {
	      i = nodes_reader[tag.reader].index;

	      group = tag.button ? 3:1;
	      if(node.group != group)
	      {
		changed = true;
		node.group = group;
	      }

	      if(node.link.target!=i)
	      {
		changed = true;
		node.link.target = i;
	      }
	    }
	    else
	    {
	      changed = true;

	      link = new Object;
	      link.id = 'L'+tag.id;
	      link.source = t;
	      link.target = nodes_reader[tag.reader].index;
	      link.value = 1

	      links.push(link);
	      linksm.push(link);

	      node = new Object;
	      node.id = 'T'+tag.id;
	      node.name = 'Tag '+tag.id;
	      node.group = 1;
	      node.index = t;
	      node.radius = 5;
	      node.link = link;
	      node.px = node.x = tag.loc[0];
	      node.py = node.y = tag.loc[1];

	      nodes_tag[tag.id] = node;
	      nodes.push(node);
	      nodesm.push(node);

	      t++;
	    }
	}

    }

  if(linksm.length)
  {
    var vislinkm = vis.selectAll('line.link')
      .data(linksm)
	.enter().append('svg:line')
	  .attr('class', 'link')
	  .attr('id', function(d)  { return d.id; })
	  .style('stroke-width', function(d) { return Math.sqrt(d.value); });
  }

  if(nodesm.length)
  {
    var visnodem = vis.selectAll('circle.node')
      .data(nodesm)
	.enter().append('svg:circle')
	  .attr('class', 'node')
	  .attr('id', function(d)  { return d.id; })
	  .attr('r', function(d) { return d.radius; })
	  .style('fill', function(d) { return fill(d.group); });

    visnodem.append('svg:title')
	  .text(function(d) { return d.name; });

  }

  if(changed)
  {
    vislink = vis.selectAll('line.link').data(links);
    visnode = vis.selectAll('circle.node').data(nodes);

    force.start();
  }
}

function refresh_svg()
{
  d3.json('http://api.openbeacon.net/get/brucon.json', process_server);

  window.setTimeout(refresh_svg, 1000);
}

refresh_svg();
