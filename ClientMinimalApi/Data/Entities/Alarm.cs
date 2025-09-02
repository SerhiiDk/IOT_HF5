namespace ClientMinimalApi.Data.Entities
{
    public class Alarm
    {
        public int Id { get; set; }
        public string Data { get; set; }
        public DateTimeOffset Date { get; set; }
        public int DeviceId { get; set; }
    }
}
