// license: cc-wrapped gpl

var MAPPER = {
  scansUrl    : 
  'https://amd.hope.net/sputnik-webapp/dynamic/dumplocations?json',
  interval    : 1000,                   // how many milliseconds between visual updates
  intervalPtr : undefined,
  tagDivs     : [],
  inAJAX      : false
};

MAPPER.convertX = function(x) {
    return Math.round((x+.82)*584/(0.52+0.82));
};

MAPPER.convertY = function(y) {
    return Math.round((-y+.7)*607/(0.7+0.68));
};

MAPPER.showScanActivity = function(data){
    var seenTags = [];
    for (var tag in data) {
        tag = data[tag];
        if (tag['z'] < 1) {
            tagDivNum = tag['tag']-3000;
	    seenTags[tagDivNum] = true;
            if (!MAPPER.tagDivs[tagDivNum]) {
		MAPPER.tagDivs[tagDivNum] = $('<div class="tag" id="tag'+tag['tag']+'"><img src="avatars/'+tag['tag']+'.png" alt="'+tag['tag']+'"></div>');
		function clickHandler(tagID) {
		    function onclick() {
			showDetails(tagID);
		    }
		    return onclick;
		}
		MAPPER.tagDivs[tagDivNum].click(clickHandler(tag['tag']));
                MAPPER.tagDivs[tagDivNum].appendTo("#map");
            }
            MAPPER.tagDivs[tagDivNum].css({
                left: MAPPER.convertX(tag['x']),
                top: MAPPER.convertY(tag['y'])
            });
            if (tag['flag']) {
                MAPPER.tagDivs[tagDivNum].addClass("flag");
            } else {
                MAPPER.tagDivs[tagDivNum].removeClass("flag");
            }
            MAPPER.tagDivs[tagDivNum].show();
        }
    }
    for (var tag in MAPPER.tagDivs) {
	if (MAPPER.tagDivs[tag] && !seenTags[tag]) {
	    MAPPER.tagDivs[tag].hide();
	}
    }
    MAPPER.inAJAX = false;
};

MAPPER.getMoreScans = function(){
    if (!MAPPER.inAJAX) {
        MAPPER.inAJAX = true;
        $.getJSON(MAPPER.scansUrl,
        MAPPER.showScanActivity
        );
    }  
};

MAPPER.init = function(){
    MAPPER.intervalPtr = setInterval( MAPPER.getMoreScans , MAPPER.interval ); 
} 


$(document).ready(function(){
  MAPPER.init();
});
