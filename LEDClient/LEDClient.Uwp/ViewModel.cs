using Flurl;
using Flurl.Http;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using Windows.UI;

namespace LEDClient.Uwp
{
    public class ViewModel : INotifyPropertyChanged
    {
        private const string Server = "http://192.168.1.71/";
        private const string LedsPath = "leds";

        private Color[] _colours = Enumerable.Range(0, 50).Select(_ => Color.FromArgb(255, 0, 0, 0)).ToArray();
        private Color _currentColour = Color.FromArgb(255, 255, 255, 255);
        private DelegateCommand _changeColorCommand;

        public event PropertyChangedEventHandler PropertyChanged;

        public ViewModel()
        {
            _changeColorCommand = new DelegateCommand(ChangeColor);

            Show();
        }

        private string FromLeds()
        {
            return string.Join("-", _colours.Select(color => $"{color.R.ToString("X2")}{color.G.ToString("X2")}{color.B.ToString("X2")}"));
        }

        private void Show()
        {
            var result = Server.AppendPathSegment(LedsPath).SetQueryParams(new { value = FromLeds() }).PostStringAsync(string.Empty).Result;
        }

        private void ChangeColor(object parameter)
        {
            switch (parameter)
            {
                case int index:
                    _colours[index] = _currentColour;
                    break;
                case string index:
                    _colours[Convert.ToInt32(index)] = _currentColour;
                    break;

                default: throw new ArgumentException("parameter");
            }

            NotifyOfPropertyChanged(nameof(Colours));

            Show();
        }

        private void NotifyOfPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public Color[] Colours => _colours;
        public ICommand ChangeColourCommand => _changeColorCommand;

        public Color CurrentColor
        {
            get { return _currentColour; }
            set
            {
                if (value != _currentColour)
                {
                    _currentColour = value;

                    NotifyOfPropertyChanged();
                }
            }
        }
    }
}
