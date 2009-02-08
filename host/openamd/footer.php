	<p><p><p><p>&nbsp;
          </td>
          <td background="images/bPanelRight.png">&nbsp;</td>
        </tr>
        <tr>
          <td><img src="images/bPanelCrnBtmLft.png" width="50" height="50" /></td>
          <td background="images/bPanelCrnBtmCnt.png">&nbsp;</td>
          <td><img src="images/bPanelCrnBtmRight.png" width="50" height="50" /></td>
        </tr>
      </table>
    </td>
    <td width="115" valign="middle">
    <!-- Floating nav -->
      <div style =  "float:left; z-index:10; position:relative; left:-200px; top:15px;">
<?php

require_once('showuser.php');
//include('config.php');


if (isset($_SESSION['error'])) {
  echo "<h2><font color=\"red\">" . $_SESSION['error'] . "</font></h2>";
  //unset($_SESSION['error']);
}

include_once 'sidebar.php';

?>
          </div>  
        </td>
      </tr>
    </table>
  </div>
</body>
</html>

