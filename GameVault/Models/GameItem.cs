using System.Windows.Media;

namespace GameVault.Models;

public class GameItem
{
    public string Name { get; set; } = string.Empty;
    public string FilePath { get; set; } = string.Empty;
    public ImageSource? Icon { get; set; }
}
