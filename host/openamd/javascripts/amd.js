var page = false;

function details(datasource) {
        if (window.XMLHttpRequest) {
                page = new XMLHttpRequest();
                page2 = new XMLHttpRequest();
        }
        else if (window.ActiveXObject) {
                page = new ActiveXObject("Microsoft.XMLHTTP");
                page2 = new ActiveXObject("Microsoft.XMLHTTP");
        }
//      document.getElementById(datasource).setAttribute("bgcolor","#E8E7E7");
        var url = "schedule_details.php?talk=" + datasource;
	if (page) {
          var obj=document.getElementById("schedule_details");
          /*obj.style.visibility='visible';*/

	      page.open("GET", url);
                page.onreadystatechange = function()
                {
                        if (page.readyState == 4 && page.status == 200) {
                                obj.innerHTML = page.responseText;
                        }
                }
                page.send(null)
        }
}
  
function close_details() {
	var littlediv = document.getElementById("schedule_details");
	littlediv.innerHTML = "";
	littlediv.style.visibility='hidden';
}
