namespace ClientMinimalApi.Data.Entities
{
    public class Devices
    {
        public int Id { get; set; }
        public string DeviceId { get; set; }
        public string Name { get; set; }
        public DateTime CreatedAt { get; set; }
        public bool Active { get; set; }
        public int LocationId { get; set; }
        public string Password { get; set; }
    }
}
